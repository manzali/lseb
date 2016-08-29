
#include "transport/libfabric/acceptor.h"

#include <iostream>
#include <cstring>

#include <rdma/fi_endpoint.h>
#include <rdma/fi_errno.h>
#include <rdma/fi_cm.h>

#include "common/exception.h"

namespace {

void read_event(fid_eq *eq, struct fi_eq_cm_entry *entry, uint32_t event) {
  struct fi_eq_err_entry err_entry;
  uint32_t ev;
  ssize_t rd;
  std::string err_msg{"Error on fi_eq_sread: "};

  rd = fi_eq_sread(eq, &ev, entry, sizeof *entry, -1, 0);
  if (rd!=sizeof *entry) {
    if (rd==-FI_EAVAIL) {
      rd = fi_eq_readerr(eq, &err_entry, 0);
      if (rd!=sizeof err_entry) {
        throw lseb::exception::acceptor::generic_error(
            "Error on fi_eq_readerr: "
                + std::string(fi_strerror(-static_cast<int>(rd))));
      } else {
        throw lseb::exception::acceptor::generic_error(
            err_msg + fi_eq_strerror(eq,
                                     err_entry.prov_errno,
                                     err_entry.err_data,
                                     nullptr,
                                     0));
      }
    } else {
      throw lseb::exception::acceptor::generic_error(err_msg + fi_strerror(-rd));
    }
  }

  if (ev!=event) {
    throw lseb::exception::acceptor::generic_error(
        err_msg + "Unexpected CM event");
  }
}

void bind_completion_queues(fid_ep *ep,
                            fid_cq **rx,
                            fid_cq **tx,
                            uint32_t size) {
  int rc = 0;
  struct fi_cq_attr cq_attr;
  lseb::Domain& d = lseb::Domain::get_instance();
  std::memset(&cq_attr, 0, sizeof cq_attr);

  cq_attr.format = FI_CQ_FORMAT_CONTEXT;
  cq_attr.wait_obj = FI_WAIT_FD;
  cq_attr.size = size + 1;

  /* Create Completion queues */
  rc = fi_cq_open(d.get_raw_domain(), &cq_attr, rx, NULL);
  if (rc) {
    throw lseb::exception::acceptor::generic_error(
        "Error on fi_cq_open rx: " + std::string(fi_strerror(-rc)));
  }

  rc = fi_cq_open(d.get_raw_domain(), &cq_attr, tx, NULL);
  if (rc) {
    throw lseb::exception::acceptor::generic_error(
        "Error on fi_cq_open tx: " + std::string(fi_strerror(-rc)));
  }

  /* Bind Completion queues */
  rc = fi_ep_bind(ep, &(*rx)->fid, FI_RECV);
  if (rc) {
    throw lseb::exception::acceptor::generic_error(
        "Error on fi_ep_bind rx: " + std::string(fi_strerror(-rc)));
  }

  rc = fi_ep_bind(ep, &(*tx)->fid, FI_SEND);
  if (rc) {
    throw lseb::exception::acceptor::generic_error(
        "Error on fi_ep_bind tx: " + std::string(fi_strerror(-rc)));
  }
}

void bind_event_queue(fid_ep *ep, fid_eq **eq) {
  int rc = 0;
  struct fi_eq_attr cm_attr;
  lseb::Domain& d = lseb::Domain::get_instance();

  std::memset(&cm_attr, 0, sizeof cm_attr);
  cm_attr.wait_obj = FI_WAIT_FD;

  rc = fi_eq_open(d.get_raw_fabric(), &cm_attr, eq, NULL);
  if (rc) {
    throw lseb::exception::acceptor::generic_error(
        "Error on fi_eq_open: " + std::string(fi_strerror(-rc)));
  }

  rc = fi_ep_bind(ep, &(*eq)->fid, 0);
  if (rc) {
    throw lseb::exception::acceptor::generic_error(
        "Error on fi_ep_bind eq: " + std::string(fi_strerror(-rc)));
  }
}

}

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

  std::unique_ptr<fi_info, fid_deleter<fi_info>> info{info_p};

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
  // TODO: Use unique_ptr on all fabric resources to avoid memory leaks on
  //       connection failure
  struct fi_eq_cm_entry entry;
  int rc = 0;
  fid_ep *ep;
  fid_cq *rx_cq;
  fid_cq *tx_cq;
  fid_eq *eq;
  Domain& d = Domain::get_instance();

  read_event(m_pep_eq.get(), &entry, FI_CONNREQ);

  rc = fi_endpoint(d.get_raw_domain(),
                   entry.info,
                   &ep,
                   NULL);
  fi_freeinfo(entry.info);
  if (rc) {
    throw exception::acceptor::generic_error(
        "Error on fi_endpoint: " + std::string(fi_strerror(-rc)));
  }

  // Create completion queues
  bind_completion_queues(ep, &rx_cq, &tx_cq, m_credits);
  // Create event queues
  bind_event_queue(ep, &eq);

  rc = fi_enable(ep);
  if (rc) {
    throw exception::acceptor::generic_error(
        "Error on fi_enable: " + std::string(fi_strerror(-rc)));
  }

  rc = fi_accept(ep, NULL, 0);
  if (rc) {
    throw exception::acceptor::generic_error(
        "Error on fi_accept: " + std::string(fi_strerror(-rc)));
  }

  read_event(eq, &entry, FI_CONNECTED);

  return make_unique<Socket>(ep, rx_cq, tx_cq, m_credits);
}

}
