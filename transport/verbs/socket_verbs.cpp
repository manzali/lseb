#include "transport/verbs/socket_verbs.h"

#include <stdexcept>

namespace lseb {

SendSocket::SendSocket(rdma_cm_id* cm_id, ibv_mr* mr, int credits)
    :
      m_cm_id(cm_id),
      m_mr(mr),
      m_credits(credits) {
}

int SendSocket::available() {
  if (m_wrs_count) {
    std::vector<ibv_wc> wcs(m_wrs_count);
    int ret = ibv_poll_cq(m_cm_id->send_cq, wcs.size(), &wcs.front());
    if (ret < 0) {
      throw std::runtime_error(
        "Error on ibv_poll_cq: " + std::string(strerror(ret)));
    }
    for (auto it = std::begin(wcs); it != std::begin(wcs) + ret; ++it) {
      if (it->status) {
        throw std::runtime_error(
          "Error status in wc of send_cq: " + std::string(
            ibv_wc_status_str(it->status)));
      }
    }
    m_wrs_count -= ret;
  }
  return m_credits - m_wrs_count;
}

size_t SendSocket::write(DataIov const& data) {

  size_t bytes_sent = 0;
  std::vector<ibv_sge> sges;
  for (auto const& iov : data) {
    ibv_sge sge;
    sge.addr = (uint64_t) (uintptr_t) iov.iov_base;
    sge.length = (uint32_t) iov.iov_len;
    sge.lkey = m_mr->lkey;
    sges.push_back(sge);
    bytes_sent += iov.iov_len;
  }

  ibv_send_wr wr;
  wr.wr_id = 0;  // Not used
  wr.next = nullptr;
  wr.sg_list = &sges.front();
  wr.num_sge = sges.size();
  wr.opcode = IBV_WR_SEND;
  wr.send_flags = 0;

  ++m_wrs_count;

  ibv_send_wr* bad_wr;
  int ret = ibv_post_send(m_cm_id->qp, &wr, &bad_wr);
  if (ret) {
    throw std::runtime_error(
      "Error on ibv_post_send: " + std::string(strerror(ret)));
  }

  return bytes_sent;
}

size_t SendSocket::write_all(std::vector<DataIov> const& data_vect) {

  std::vector<std::pair<ibv_send_wr, std::vector<ibv_sge> > > wrs(
    data_vect.size());
  size_t bytes_sent = 0;

  for (int i = 0; i < wrs.size(); ++i) {

    // Get references
    ibv_send_wr& wr = wrs[i].first;
    std::vector<ibv_sge>& sges = wrs[i].second;
    DataIov const& data = data_vect[i];

    for (auto const& iov : data) {
      ibv_sge sge;
      sge.addr = (uint64_t) (uintptr_t) iov.iov_base;
      sge.length = (uint32_t) iov.iov_len;
      sge.lkey = m_mr->lkey;
      sges.push_back(sge);
      bytes_sent += iov.iov_len;
    }

    wr.wr_id = i;  // Not used
    wr.next = (i + 1 == wrs.size()) ? nullptr : &(wrs[i + 1].first);
    wr.sg_list = &sges.front();
    wr.num_sge = sges.size();
    wr.opcode = IBV_WR_SEND;
    wr.send_flags = 0;

    ++m_wrs_count;
  }

  if (!wrs.empty()) {
    ibv_send_wr* bad_wr;
    int ret = ibv_post_send(m_cm_id->qp, &(wrs.front().first), &bad_wr);
    if (ret) {
      throw std::runtime_error(
        "Error on ibv_post_send: " + std::string(strerror(ret)));
    }
  }

  return bytes_sent;
}

RecvSocket::RecvSocket(rdma_cm_id* cm_id, ibv_mr* mr, int credits)
    :
      m_cm_id(cm_id),
      m_mr(mr),
      m_credits(credits),
      m_init(false) {
}

iovec RecvSocket::read() {
  ibv_wc wc;
  int ret = 0;
  do {
    ret = ibv_poll_cq(m_cm_id->recv_cq, 1, &wc);
  } while (!ret);
  if (ret < 0) {
    throw std::runtime_error(
      "Error on ibv_poll_cq: " + std::string(strerror(ret)));
  }
  if (wc.status) {
    throw std::runtime_error(
      "Error status in wc of recv_cq: " + std::string(
        ibv_wc_status_str(wc.status)));
  }
  iovec iov = { (void*) wc.wr_id, wc.byte_len };
  return iov;
}

std::vector<iovec> RecvSocket::read_all() {
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
    iov_vect.push_back( { (void*) it->wr_id, it->byte_len });
  }

  return iov_vect;
}

void RecvSocket::release(std::vector<iovec> const& iov_vect) {

  std::vector<std::pair<ibv_recv_wr, ibv_sge> > wrs(iov_vect.size());

  for (int i = 0; i < wrs.size(); ++i) {
    iovec& iov = iov_vect[i];
    ibv_sge& sge = wrs[i].second;
    sge.addr = (uint64_t) (uintptr_t) iov.iov_base;
    sge.length = (uint32_t) iov.iov_len;
    sge.lkey = m_mr->lkey;

    ibv_recv_wr& wr = wrs[i].first;
    wr.wr_id = (uint64_t) (uintptr_t) iov.iov_base;
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
    if (rdma_accept(m_cm_id, NULL)) {
      throw std::runtime_error(
        "Error on rdma_accept: " + std::string(strerror(errno)));
    }
    m_init = true;
  }
}

}
