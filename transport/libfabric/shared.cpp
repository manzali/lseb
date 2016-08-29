//
// Created by Valentino on 29/08/16.
//

#include "shared.h"

#include "rdma/fi_errno.h"

#include "Domain.h"
#include "common/exception.h"


namespace lseb {

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
      throw lseb::exception::acceptor::generic_error(
          err_msg + fi_strerror(-rd));
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

