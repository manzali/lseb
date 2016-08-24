#include "transport/libfabric/connector.h"

#include "rdma/fi_endpoint.h"
#include "rdma/fi_cm.h"

#include "common/exception.h"
#include "domain.h"

namespace lseb {

Connector::Connector(int credits) : m_credits(credits) {}

std::unique_ptr<Socket> Connector::connect(
    std::string const& hostname,
    std::string const& port) {
  // TODO: Add subfunctions to make this function more readable
  // TODO: Add Named Return Value Optimization
  // TODO: Use unique_ptr on all fabric resources to avoid memory leaks on
  //       connection failure
  // TODO: Improve error reporting (and avoid exceptions?)
  fi_info *info;

  struct fi_eq_cm_entry entry;
  uint32_t event;
  int rc = 0;
  ssize_t rd;

  /* Resolve address to a fabric specific one */
  // TODO: Verify address correctness in info->dest_addrs
  fi_getinfo(FI_VERSION(1, 3),
             hostname.c_str(),
             port.c_str(),
             0,
             nullptr,
             &info);

  Domain& d = Domain::get_instance();
  fid_eq *eq;
  /* EQ create */
  struct fi_eq_attr cm_attr;

  memset(&cm_attr, 0, sizeof cm_attr);
  cm_attr.wait_obj = FI_WAIT_FD;

  rc = fi_eq_open(d.get_raw_fabric(), &cm_attr, &eq, NULL);
  if (rc) {
    //FT_PRINTERR("fi_eq_open", rc);
  }

  /* End CQ create*/
  fid_ep *ep;
  /* Open endpoint */
  rc = fi_endpoint(d.get_raw_domain(), d.get_info(), &ep, NULL);
  if (rc) {
    //FT_PRINTERR("fi_endpoint", rc);
    return nullptr;
  }

  fid_cq *rx_cq;
  fid_cq *tx_cq;
  /* Create event queue */
  struct fi_cq_attr cq_attr;
  memset(&cq_attr, 0, sizeof cq_attr);
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

  /* Bind eq to ep */
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

  printf("Connecting to server\n");

  rc = fi_connect(ep, info->dest_addr, NULL, 0);
  fi_freeinfo(info);
  if (rc) {
    // FT_PRINTERR("fi_connect", rc);
    return nullptr;
  }

  rd = fi_eq_sread(eq, &event, &entry, sizeof entry, -1, 0);
  if (rd!=sizeof entry) {
    // FT_PROCESS_EQ_ERR(rd, ctx->eq, "fi_eq_sread", "connect");
    return nullptr;
  }

  if (event!=FI_CONNECTED) {
    fprintf(stderr, "Unexpected CM event %d\n", event);
    return nullptr;
  }

  printf("Connection successful\n");
  // std::unique_ptr<Socket> socket_ptr(new Socket(cm_id, m_credits));
  return lseb::make_unique<Socket>(ep, rx_cq, tx_cq, m_credits);
}

}
