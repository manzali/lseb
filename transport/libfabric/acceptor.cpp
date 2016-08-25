
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
  // TODO: Use fi_getinfo with FI_SOURCE to specify that the node and service
  // parameters specify the local source address to associate with an endpoint
  if (m_pep)
    return;
  fid_pep *pep{nullptr};
  fi_info *info{nullptr};
  // NOTE: Works for verbs, not necessarily for psm: needs investigation.
  //       Probably works for psm only with FI_PSM_NAME_SERVER enabled.
  int rc = fi_getinfo(FI_VERSION(1, 3),
                      hostname.c_str(),
                      port.c_str(),
                      FI_SOURCE,
                      nullptr,
                      &info);

  if (rc) {
    std::cerr << fi_strerror(-rc) << "\n";
  }

  Domain& domain = Domain::get_instance();
  rc = fi_passive_ep(domain.get_raw_fabric(),
                     info,
                     &pep,
                     NULL /* context */);
  if (rc) {
    std::cerr << "Unable to open listener endpoint\n";
  }

  m_pep.reset(pep);

  fid_eq *pep_eq;
  struct fi_eq_attr cm_attr;

  cm_attr.wait_obj = FI_WAIT_FD;

  rc = fi_eq_open(domain.get_raw_fabric(), &cm_attr, &pep_eq, NULL);

  if (rc) {
    std::cerr << fi_strerror(-rc) << "\n";
  }

  m_pep_eq.reset(pep_eq);

  rc = fi_pep_bind(m_pep.get(), &m_pep_eq.get()->fid, 0);
  if (rc) {
    std::cerr << "fi_pep_bind" << fi_strerror(-rc) << "\n";
  }

  rc = fi_listen(m_pep.get());
  if (rc) {
    std::cerr << fi_strerror(-rc) << "\n";
  }
  fi_freeinfo(info);
}

std::unique_ptr<Socket> Acceptor::accept() {
  // TODO: Add subfunctions to make this function more readable
  // TODO: Add Named Return Value Optimization
  // TODO: Use unique_ptr on all fabric resources to avoid memory leaks on
  //       connection failure
  // TODO: Improve error reporting (and avoid exceptions?)
  struct fi_eq_cm_entry entry;
  uint32_t event;
  int rc = 0;
  ssize_t rd;

  rd = fi_eq_sread(m_pep_eq.get(), &event, &entry, sizeof entry, -1, 0);
  if (rd!=sizeof entry) {
    // FT_PROCESS_EQ_ERR(rd, ctx->eq, "fi_eq_sread", "listen");
    return nullptr;
  }

  if (event!=FI_CONNREQ) {
    fprintf(stderr, "Unexpected CM event %d\n", event);
    return nullptr;
  }

  Domain& d = Domain::get_instance();

  fid_ep *ep;
  fid_cq *rx_cq;
  fid_cq *tx_cq;
  fid_eq *eq;
  rc = fi_endpoint(d.get_raw_domain(),
                   entry.info,
                   &ep,
                   NULL);
  if (rc) {
    // FT_PRINTERR("fi_endpoint", rc);
    return nullptr;
  }
  fi_freeinfo(entry.info);

  /* Create completion queue */
  struct fi_cq_attr cq_attr;
  std::memset(&cq_attr, 0, sizeof cq_attr);

  cq_attr.format = FI_CQ_FORMAT_CONTEXT;
  cq_attr.wait_obj = FI_WAIT_FD;

  cq_attr.size = m_credits + 1;

  rc = fi_cq_open(d.get_raw_domain(), &cq_attr, &rx_cq, NULL);
  if (rc) {
    // FT_PRINTERR("fi_cq_open", rc);
    return nullptr;
  }

  rc = fi_cq_open(d.get_raw_domain(), &cq_attr, &tx_cq, NULL);
  if (rc) {
    // FT_PRINTERR("fi_cq_open", rc);
    return nullptr;
  }

  /* Bind Completion queue */
  rc = fi_ep_bind(ep, &rx_cq->fid, FI_RECV);
  if (rc) {
    // FT_PRINTERR("fi_ep_bind", rc);
    return nullptr;
  }

  rc = fi_ep_bind(ep, &tx_cq->fid, FI_SEND);
  if (rc) {
    // FT_PRINTERR("fi_ep_bind", rc);
    return nullptr;
  }

  struct fi_eq_attr cm_attr;

  std::memset(&cm_attr, 0, sizeof cm_attr);
  cm_attr.wait_obj = FI_WAIT_FD;

  rc = fi_eq_open(d.get_raw_fabric(), &cm_attr, &eq, NULL);
  if (rc) {
    // FT_PRINTERR("fi_ep_bind", rc);
    return nullptr;
  }

  rc = fi_ep_bind(ep, &eq->fid, 0);
  if (rc) {
    // FT_PRINTERR("fi_ep_bind", rc);
    return nullptr;
  }

  rc = fi_enable(ep);
  if (rc) {
    // FT_PRINTERR("fi_enable", rc);
    return nullptr;
  }

  rc = fi_accept(ep, NULL, 0);
  if (rc) {
    // FT_PRINTERR("fi_accept", rc);
    return nullptr;
  }

  rd = fi_eq_sread(eq, &event, &entry, sizeof entry, -1, 0);
  if (rd!=sizeof entry) {
    // FT_PROCESS_EQ_ERR(rd, ctx->eq, "fi_eq_sread", "accept");
    return nullptr;
  }

  if (event!=FI_CONNECTED) {
    fprintf(stderr, "Unexpected CM event %d\n", event);
    return nullptr;
  }
  printf("Connection accepted\n");

  return lseb::make_unique<Socket>(ep, rx_cq, tx_cq, m_credits);
}

}
