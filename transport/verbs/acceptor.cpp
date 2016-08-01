#include "transport/verbs/acceptor.h"

#include "common/exception.h"

namespace {

rdma_addrinfo* create_addr_info(
    std::string const& hostname,
    std::string const& port) {
  rdma_addrinfo hints;
  memset(&hints, 0, sizeof hints);
  hints.ai_flags = RAI_PASSIVE;
  hints.ai_port_space = RDMA_PS_TCP;
  rdma_addrinfo* res;
  int ret = rdma_getaddrinfo(
      const_cast<char*>(hostname.c_str()),
      const_cast<char*>(port.c_str()),
      &hints,
      &res);
  if (ret) {
    throw lseb::exception::acceptor::generic_error(
        "Error on rdma_getaddrinfo: " + std::string(strerror(errno)));
  }
  return res;
}

void destroy_addr_info(rdma_addrinfo* res) {
  rdma_freeaddrinfo(res);
}

}

namespace lseb {

Acceptor::Acceptor(int credits)
    : m_cm_id(nullptr) {
  m_credits = credits;
  m_min_rtr_timer = verbs::MIN_RTR_TIMER;
  m_rnr_retry_count = verbs::RNR_RETRY_COUNT;
}

Acceptor::~Acceptor() {
  rdma_destroy_ep(m_cm_id);
}

void Acceptor::listen(std::string const& hostname, std::string const& port) {
  auto res = create_addr_info(hostname, port);
  ibv_qp_init_attr init_attr;
  memset(&init_attr, 0, sizeof(init_attr));
  init_attr.cap.max_send_wr = m_credits;
  init_attr.cap.max_recv_wr = m_credits;
  init_attr.cap.max_send_sge = 1;
  init_attr.cap.max_recv_sge = 1;
  init_attr.cap.max_inline_data = 0;
  init_attr.sq_sig_all = 1;
  init_attr.qp_type = IBV_QPT_RC;
  int ret = rdma_create_ep(&m_cm_id, res, NULL, &init_attr);
  destroy_addr_info(res);
  if (ret) {
    rdma_destroy_ep(m_cm_id);
    throw exception::acceptor::generic_error(
        "Error on rdma_create_ep: " + std::string(strerror(errno)));
  }

  if (rdma_listen(m_cm_id, 128)) {
    rdma_destroy_ep(m_cm_id);
    throw exception::acceptor::generic_error(
        "Error on rdma_listen: " + std::string(strerror(errno)));
  }
}

std::unique_ptr<Socket> Acceptor::accept() {
  rdma_cm_id* new_cm_id;
  if (rdma_get_request(m_cm_id, &new_cm_id)) {
    throw exception::acceptor::generic_error(
        "Error on rdma_get_request: " + std::string(strerror(errno)));
  }

  rdma_conn_param conn_param;
  memset(&conn_param, 0, sizeof(rdma_conn_param));
  conn_param.rnr_retry_count = m_rnr_retry_count;
  if (rdma_accept(new_cm_id, &conn_param)) {
    rdma_destroy_ep(new_cm_id);
    throw exception::acceptor::generic_error(
        "Error on rdma_accept: " + std::string(strerror(errno)));
  }

  ibv_qp_attr attr;
  memset(&attr, 0, sizeof(ibv_qp_attr));
  attr.min_rnr_timer = m_min_rtr_timer;
  int flags = IBV_QP_MIN_RNR_TIMER;
  if (ibv_modify_qp(new_cm_id->qp, &attr, flags)) {
    rdma_destroy_ep(new_cm_id);
    throw exception::acceptor::generic_error(
        "Error on ibv_modify_qp: " + std::string(strerror(errno)));
  }

  std::unique_ptr<Socket> socket_ptr(new Socket(new_cm_id, m_credits));
  return socket_ptr;
}

}
