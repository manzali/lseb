#ifndef TRANSPORT_VERBS_SOCKET_VERBS_H
#define TRANSPORT_VERBS_SOCKET_VERBS_H

#include <cstring>

#include <infiniband/verbs.h>
#include <rdma/rdma_verbs.h>

#include "common/utility.h"

namespace lseb {

class Socket {
 public:
  Socket() {

  }
};

class SendSocket : public Socket {
  rdma_cm_id* m_cm_id;
  ibv_mr* m_mr;
  int m_credits;
  int m_wrs_count;

 public:
  SendSocket(rdma_cm_id* cm_id, ibv_mr* mr, int credits);
  int available();
  size_t write(DataIov const& data);
  size_t write_all(std::vector<DataIov> const& data_vect);

  static ibv_qp_init_attr create_qp_attr(int credits) {
    ibv_qp_init_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.cap.max_send_wr = credits;
    attr.cap.max_send_sge = 2;
    attr.cap.max_recv_wr = 1;
    attr.cap.max_recv_sge = 1;
    attr.sq_sig_all = 1;
    attr.qp_type = IBV_QPT_RC;
    return attr;
  }
};

class RecvSocket : public Socket {
  rdma_cm_id* m_cm_id;
  ibv_mr* m_mr;
  int m_credits;
  bool m_init;

 public:
  RecvSocket(rdma_cm_id* cm_id, ibv_mr* mr, int credits);
  iovec read();
  std::vector<iovec> read_all();
  void release(std::vector<iovec> const& iov_vect);

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
