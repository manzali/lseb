#ifndef TRANSPORT_VERBS_SOCKET_VERBS_H
#define TRANSPORT_VERBS_SOCKET_VERBS_H

#include <cstring>

#include <infiniband/verbs.h>
#include <rdma/rdma_verbs.h>

#include "common/utility.h"

namespace lseb {

class SendSocket {
  rdma_cm_id* m_cm_id;
  ibv_mr* m_mr;
  int m_credits;
  int m_counter;

 public:
  SendSocket(rdma_cm_id* cm_id, ibv_mr* mr, int credits);
  std::vector<iovec> pop_completed();
  size_t post_write(iovec const& iov);
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
  void register_memory(void* buffer, size_t size);
  std::vector<iovec> pop_completed();
  void post_read(iovec const& iov);
  void post_read(std::vector<iovec> const& iov_vect);

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
