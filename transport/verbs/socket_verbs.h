#ifndef TRANSPORT_VERBS_SOCKET_VERBS_H
#define TRANSPORT_VERBS_SOCKET_VERBS_H

#include <map>
#include <vector>
#include <string>

#include <cstring>

#include <infiniband/verbs.h>
#include <rdma/rdma_verbs.h>

namespace lseb {

namespace verbs {
static uint8_t RETRY_COUNT = 7;
static uint8_t RNR_RETRY_COUNT = 7;
static uint8_t MIN_RTR_TIMER = 1;
}

class SendSocket {
  rdma_cm_id* m_cm_id;
  ibv_mr* m_mr;
  int m_credits;
  std::map<void*, size_t> m_wrs_size;

 public:
  SendSocket(rdma_cm_id* cm_id, int credits);
  ~SendSocket();
  void register_memory(void* buffer, size_t size);
  std::vector<iovec> pop_completed();
  void post_send(iovec const& iov);
  int pending();

  static ibv_qp_init_attr create_qp_attr(int credits) {
    ibv_qp_init_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.cap.max_send_wr = credits;
    attr.cap.max_send_sge = 1;
    attr.cap.max_recv_wr = 1;
    attr.cap.max_recv_sge = 1;
    attr.sq_sig_all = 1;
    attr.qp_type = IBV_QPT_RC;
    return attr;
  }
};

class RecvSocket {
  rdma_cm_id* m_cm_id;
  ibv_mr* m_mr;
  int m_credits;
  bool m_init;

 public:
  RecvSocket(rdma_cm_id* cm_id, int credits);
  ~RecvSocket();
  void register_memory(void* buffer, size_t size);
  std::vector<iovec> pop_completed();
  void post_recv(iovec const& iov);
  void post_recv(std::vector<iovec> const& iov_vect);
  std::string peer_hostname();

  static ibv_qp_init_attr create_qp_attr(int credits) {
    ibv_qp_init_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.cap.max_send_wr = 1;
    attr.cap.max_send_sge = 1;
    attr.cap.max_recv_wr = credits;
    attr.cap.max_recv_sge = 1;
    attr.sq_sig_all = 1;
    attr.qp_type = IBV_QPT_RC;
    return attr;
  }
};

}

#endif
