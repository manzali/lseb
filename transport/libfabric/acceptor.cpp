
#include "transport/libfabric/acceptor.h"

#include <iostream>
#include <cstring>

#include <rdma/fi_endpoint.h>
#include <rdma/fi_errno.h>
#include <rdma/fi_cm.h>

#include "common/exception.h"

namespace lseb {

Acceptor::Acceptor(int credits)
    : m_credits(credits),
      m_pep(nullptr),
      m_pep_eq(nullptr) {
}

void Acceptor::listen(std::string const& hostname, std::string const& port) {
  fi_info *info_p{nullptr};
  fid_pep *pep{nullptr};
  fid_eq *pep_eq{nullptr};
  Domain& domain = Domain::get_instance();
  struct fi_eq_attr eq_attr;

  if (m_pep)
    return;

  // NOTE: Works for verbs, not necessarily for psm: needs investigation.
  //       Probably works for psm only with FI_PSM_NAME_SERVER enabled.

  // Resolve hostname and port to fabric specific addresses
  int rc = fi_getinfo(FI_VERSION(1, 3),
                      hostname.c_str(),
                      port.c_str(),
                      FI_SOURCE,
                      domain.get_hints(),
                      &info_p);

  if (rc) {
    throw exception::acceptor::generic_error(
        "Error on fi_getinfo: " + std::string(fi_strerror(-rc)));
  }

  fabric_ptr<fi_info> info{info_p};

  rc = fi_passive_ep(domain.get_raw_fabric(),
                     info.get(),
                     &pep,
                     NULL /* context */);
  if (rc) {
    std::cerr << "Unable to open listener endpoint" << std::endl;
    throw exception::acceptor::generic_error(
        "Error on fi_passive_ep: " + std::string(fi_strerror(-rc)));
  }

  m_pep.reset(pep);

  std::memset(&eq_attr, 0, sizeof eq_attr);
  eq_attr.wait_obj = FI_WAIT_FD;

  rc = fi_eq_open(domain.get_raw_fabric(), &eq_attr, &pep_eq, NULL);

  if (rc) {
    throw exception::acceptor::generic_error(
        "Error on fi_eq_open: " + std::string(fi_strerror(-rc)));
  }

  m_pep_eq.reset(pep_eq);

  rc = fi_pep_bind(m_pep.get(), &m_pep_eq.get()->fid, 0);
  if (rc) {
    throw exception::acceptor::generic_error(
        "Error on fi_pep_bind: " + std::string(fi_strerror(-rc)));
  }

  rc = fi_listen(m_pep.get());
  if (rc) {
    throw exception::acceptor::generic_error(
        "Error on fi_listen: " + std::string(fi_strerror(-rc)));
  }
}

std::unique_ptr<Socket> Acceptor::accept() {
  struct fi_eq_cm_entry entry;
  int rc = 0;
  fid_ep *ep_raw;
  fabric_ptr<fid_ep> ep;
  fabric_ptr<fid_cq> rx_cq, tx_cq;
  fabric_ptr<fid_eq> eq;

  Domain& d = Domain::get_instance();

  read_event(m_pep_eq, &entry, FI_CONNREQ);

  rc = fi_endpoint(d.get_raw_domain(),
                   entry.info,
                   &ep_raw,
                   NULL);
  fi_freeinfo(entry.info);
  if (rc) {
    throw exception::acceptor::generic_error(
        "Error on fi_endpoint: " + std::string(fi_strerror(-rc)));
  }
  ep.reset(ep_raw);

  // Create completion queues
  bind_completion_queues(ep, rx_cq, tx_cq, m_credits);
  // Create event queues
  bind_event_queue(ep, eq);

  rc = fi_enable(ep.get());
  if (rc) {
    throw exception::acceptor::generic_error(
        "Error on fi_enable: " + std::string(fi_strerror(-rc)));
  }

  rc = fi_accept(ep.get(), NULL, 0);
  if (rc) {
    throw exception::acceptor::generic_error(
        "Error on fi_accept: " + std::string(fi_strerror(-rc)));
  }

  read_event(eq, &entry, FI_CONNECTED);

  return make_unique<Socket>(ep.release(),
                             rx_cq.release(),
                             tx_cq.release(),
                             m_credits);
}

}
