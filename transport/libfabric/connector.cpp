#include "transport/libfabric/connector.h"

#include <rdma/fi_errno.h>
#include "rdma/fi_endpoint.h"
#include "rdma/fi_cm.h"

#include "common/exception.h"
#include "domain.h"

namespace lseb {

Connector::Connector(int credits) : m_credits(credits) {}

std::unique_ptr<Socket> Connector::connect(
    std::string const& hostname,
    std::string const& port) {
  struct fi_eq_cm_entry entry;
  int rc = 0;
  fi_info *info_p;
  fid_ep *ep_raw;
  fabric_ptr<fid_ep> ep;
  fabric_ptr<fid_cq> rx_cq, tx_cq;
  fabric_ptr<fid_eq> eq;

  Domain& d = Domain::get_instance();

  /* Resolve address to a fabric specific one */
  // TODO: Verify address correctness in info->dest_addrs
  rc = fi_getinfo(FI_VERSION(1, 3),
                  hostname.c_str(),
                  port.c_str(),
                  0,
                  d.get_hints(),
                  &info_p);

  if (rc) {
    throw exception::connector::generic_error(
        "Error on fi_getinfo: " + std::string(fi_strerror(-rc)));
  }

  fabric_ptr<fi_info> info{info_p};

  rc = fi_endpoint(d.get_raw_domain(), info.get(), &ep_raw, NULL);

  if (rc) {
    throw exception::connector::generic_error(
        "Error on fi_endpoint: " + std::string(fi_strerror(-rc)));
  }
  ep.reset(ep_raw);

  // Create completion queues
  bind_completion_queues(ep, rx_cq, tx_cq, m_credits);
  // Create event queues
  bind_event_queue(ep, eq);


  rc = fi_connect(ep.get(), info->dest_addr, NULL, 0);
  if (rc) {
    throw exception::connector::generic_error(
        "Error on fi_connect: " + std::string(fi_strerror(-rc)));
  }

  read_event(eq, &entry, FI_CONNECTED);

  return make_unique<Socket>(ep.release(),
                             rx_cq.release(),
                             tx_cq.release(),
                             m_credits);
}

}
