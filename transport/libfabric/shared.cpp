#include "shared.h"

#include <cstring> // std::memset

#include "rdma/fi_errno.h"

#include "domain.h"
#include "common/exception.h"

namespace lseb {

void read_event(
    fabric_ptr<fid_eq> const& eq,
    fi_eq_cm_entry* entry,
    uint32_t event) {

  fi_eq_err_entry err_entry;
  uint32_t ev;
  ssize_t rd;
  std::string err_msg { "Error on fi_eq_sread: " };

  rd = fi_eq_sread(eq.get(), &ev, entry, sizeof(*entry), -1, 0);
  if (rd != sizeof(*entry)) {
    if (rd == -FI_EAVAIL) {
      rd = fi_eq_readerr(eq.get(), &err_entry, 0);
      if (rd != sizeof(err_entry)) {
        throw lseb::exception::connection::generic_error(
            "Error on fi_eq_readerr: "
                + std::string(fi_strerror(-static_cast<int>(rd))));
      } else {
        throw lseb::exception::connection::generic_error(
            err_msg
                + fi_eq_strerror(
                    eq.get(),
                    err_entry.prov_errno,
                    err_entry.err_data,
                    nullptr,
                    0));
      }
    } else {
      throw lseb::exception::connection::generic_error(
          err_msg + fi_strerror(-rd));
    }
  }

  if (ev != event) {
    throw lseb::exception::connection::generic_error(
        err_msg + "Unexpected CM event");
  }
}

void bind_completion_queues(
    fabric_ptr<fid_ep> const& ep,
    fabric_ptr<fid_cq>& rx,
    fabric_ptr<fid_cq>& tx,
    uint32_t size) {

  lseb::Domain& d = lseb::Domain::get_instance();

  fi_cq_attr cq_attr;
  std::memset(&cq_attr, 0, sizeof cq_attr);

  cq_attr.format = FI_CQ_FORMAT_CONTEXT;
  cq_attr.wait_obj = FI_WAIT_NONE;
  cq_attr.size = size;

  /* Create Completion queues */
  fid_cq *rx_raw, *tx_raw;
  int rc = fi_cq_open(d.get_raw_domain(), &cq_attr, &rx_raw, NULL);
  if (rc) {
    throw lseb::exception::connection::generic_error(
        "Error on fi_cq_open rx: " + std::string(fi_strerror(-rc)));
  }
  rx.reset(rx_raw);

  rc = fi_cq_open(d.get_raw_domain(), &cq_attr, &tx_raw, NULL);
  if (rc) {
    throw lseb::exception::connection::generic_error(
        "Error on fi_cq_open tx: " + std::string(fi_strerror(-rc)));
  }
  tx.reset(tx_raw);

  /* Bind Completion queues */
  rc = fi_ep_bind(ep.get(), &rx->fid, FI_RECV);
  if (rc) {
    throw lseb::exception::connection::generic_error(
        "Error on fi_ep_bind rx: " + std::string(fi_strerror(-rc)));
  }

  rc = fi_ep_bind(ep.get(), &tx->fid, FI_SEND);
  if (rc) {
    throw lseb::exception::connection::generic_error(
        "Error on fi_ep_bind tx: " + std::string(fi_strerror(-rc)));
  }
}

void bind_event_queue(fabric_ptr<fid_ep> const& ep, fabric_ptr<fid_eq>& eq) {

  lseb::Domain& d = lseb::Domain::get_instance();

  fi_eq_attr cm_attr;
  std::memset(&cm_attr, 0, sizeof cm_attr);
  cm_attr.wait_obj = FI_WAIT_FD;

  fid_eq* eq_raw;
  int rc = fi_eq_open(d.get_raw_fabric(), &cm_attr, &eq_raw, NULL);
  if (rc) {
    throw lseb::exception::connection::generic_error(
        "Error on fi_eq_open: " + std::string(fi_strerror(-rc)));
  }
  eq.reset(eq_raw);

  rc = fi_ep_bind(ep.get(), &eq->fid, 0);
  if (rc) {
    throw lseb::exception::connection::generic_error(
        "Error on fi_ep_bind eq: " + std::string(fi_strerror(-rc)));
  }
}

}

