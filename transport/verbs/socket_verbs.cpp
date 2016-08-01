#include "transport/verbs/socket_verbs.h"

#include <stdexcept>
#include <arpa/inet.h>

namespace lseb {

SendSocket::SendSocket(rdma_cm_id* cm_id, int credits)
    :
      m_cm_id(cm_id),
      m_mr(nullptr),
      m_credits(credits) {
}

SendSocket::~SendSocket() {
  // To be filled
}

void SendSocket::register_memory(void* buffer, size_t size) {
  m_mr = rdma_reg_msgs(m_cm_id, buffer, size);
  if (!m_mr) {
    throw std::runtime_error(
      "Error on rdma_reg_msgs: " + std::string(strerror(errno)));
  }
}

std::vector<iovec> SendSocket::pop_completed() {

  std::vector<ibv_wc> wcs(m_credits);
  int ret = ibv_poll_cq(m_cm_id->send_cq, wcs.size(), &wcs.front());
  if (ret < 0) {
    throw std::runtime_error(
      "Error on ibv_poll_cq: " + std::string(strerror(ret)));
  }
  std::vector<iovec> vect;
  for (auto wcs_it = std::begin(wcs); wcs_it != std::begin(wcs) + ret;
      ++wcs_it) {
    if (wcs_it->status) {
      throw std::runtime_error(
        "Error status in wc of send_cq: " + std::string(
          ibv_wc_status_str(wcs_it->status)));
    }
    auto map_it = m_wrs_size.find(reinterpret_cast<void*>(wcs_it->wr_id));
    if (map_it == std::end(m_wrs_size)){
      throw std::runtime_error("Error on erase: key element not exists");
    }
    vect.push_back( {map_it->first, map_it->second});
    m_wrs_size.erase(map_it);
  }

  return vect;
}

void SendSocket::post_send(iovec const& iov) {

  ibv_sge sge;
  sge.addr = reinterpret_cast<uint64_t>(iov.iov_base);
  sge.length = iov.iov_len;
  sge.lkey = m_mr->lkey;

  ibv_send_wr wr;
  wr.wr_id = reinterpret_cast<uint64_t>(iov.iov_base);
  wr.next = nullptr;
  wr.sg_list = &sge;
  wr.num_sge = 1;
  wr.opcode = IBV_WR_SEND;
  wr.send_flags = 0;

  ibv_send_wr* bad_wr;
  int ret = ibv_post_send(m_cm_id->qp, &wr, &bad_wr);
  if (ret) {
    throw std::runtime_error(
      "Error on ibv_post_send: " + std::string(strerror(ret)));
  }

  auto p = m_wrs_size.insert(
      std::pair<void*, size_t>(iov.iov_base, iov.iov_len));
   if (!p.second) {
     throw std::runtime_error("Error on insert: key element already exists");
   }
}

int SendSocket::pending() {
  return m_wrs_size.size();
}

RecvSocket::RecvSocket(rdma_cm_id* cm_id, int credits)
    :
      m_cm_id(cm_id),
      m_mr(nullptr),
      m_credits(credits),
      m_init(false) {
}

RecvSocket::~RecvSocket() {
  // To be filled
}

void RecvSocket::register_memory(void* buffer, size_t size) {
  m_mr = rdma_reg_msgs(m_cm_id, buffer, size);
  if (!m_mr) {
    throw std::runtime_error(
      "Error on rdma_reg_msgs: " + std::string(strerror(errno)));
  }
}

std::vector<iovec> RecvSocket::pop_completed() {
  std::vector<ibv_wc> wcs(m_credits);
  int ret = ibv_poll_cq(m_cm_id->recv_cq, wcs.size(), &wcs.front());
  if (ret < 0) {
    throw std::runtime_error(
      "Error on ibv_poll_cq: " + std::string(strerror(ret)));
  }

  std::vector<iovec> iov_vect;
  for (auto it = std::begin(wcs); it != std::begin(wcs) + ret; ++it) {
    if (it->status) {
      throw std::runtime_error(
        "Error status in wc of recv_cq: " + std::string(
          ibv_wc_status_str(it->status)));
    }
    iov_vect.push_back(
      { reinterpret_cast<void*>(it->wr_id), it->byte_len });
  }

  return iov_vect;
}

void RecvSocket::post_recv(iovec const& iov) {
  post_recv(std::vector<iovec>(1, iov));
}

void RecvSocket::post_recv(std::vector<iovec> const& iov_vect) {

  std::vector<std::pair<ibv_recv_wr, ibv_sge> > wrs(iov_vect.size());

  for (int i = 0; i < wrs.size(); ++i) {
    iovec const& iov = iov_vect[i];
    ibv_sge& sge = wrs[i].second;
    sge.addr = reinterpret_cast<uint64_t>(iov.iov_base);
    sge.length = iov.iov_len;
    sge.lkey = m_mr->lkey;

    ibv_recv_wr& wr = wrs[i].first;
    wr.wr_id = reinterpret_cast<uint64_t>(iov.iov_base);
    wr.next = (i + 1 == wrs.size()) ? nullptr : &(wrs[i + 1].first);
    wr.sg_list = &sge;
    wr.num_sge = 1;
  }

  if (!wrs.empty()) {
    ibv_recv_wr* bad_wr;
    int ret = ibv_post_recv(m_cm_id->qp, &(wrs.front().first), &bad_wr);
    if (ret) {
      throw std::runtime_error(
        "Error on ibv_post_recv: " + std::string(strerror(ret)));
    }
  }

  if (!m_init) {
    rdma_conn_param conn_param;
    memset(&conn_param, 0, sizeof(rdma_conn_param));
    conn_param.rnr_retry_count = verbs::RNR_RETRY_COUNT;
    if (rdma_accept(m_cm_id, &conn_param)) {
      throw std::runtime_error(
        "Error on rdma_accept: " + std::string(strerror(errno)));
    }

    ibv_qp_attr attr;
    memset(&attr, 0, sizeof(ibv_qp_attr));
    attr.min_rnr_timer = verbs::MIN_RTR_TIMER;
    int flags = IBV_QP_MIN_RNR_TIMER;
    if (ibv_modify_qp(m_cm_id->qp, &attr, flags)) {
      throw std::runtime_error(
          "Error on ibv_modify_qp: " + std::string(strerror(errno)));
    }

    m_init = true;
  }
}

std::string RecvSocket::peer_hostname() {
  char str[INET_ADDRSTRLEN];
  auto addr = reinterpret_cast<sockaddr_in*>(rdma_get_peer_addr(m_cm_id));
  inet_ntop(AF_INET, &(addr->sin_addr), str, INET_ADDRSTRLEN);
  return str;
}

}
