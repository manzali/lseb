#include "transport/verbs/socket.h"

#include <arpa/inet.h>
#include "common/exception.h"

namespace {
std::vector<ibv_mr*>::const_iterator find_mr(
    std::vector<ibv_mr*> const& mrs,
    iovec const& iov) {
  auto it = std::begin(mrs);
  for (; it != std::end(mrs); ++it) {
    ibv_mr* mr = *it;
    if (mr->addr <= iov.iov_base
        && mr->addr + mr->length >= iov.iov_base + iov.iov_len) {
      break;
    }
  }
  return it;
}
}

namespace lseb {

Socket::Socket(rdma_cm_id* cm_id, uint32_t credits)
    : m_cm_id(cm_id),
      m_credits(credits),
      m_comp_send(m_credits),
      m_comp_recv(m_credits) {
}

Socket::~Socket() {
  auto it = std::begin(m_mrs);
  for (; it != std::end(m_mrs); ++it) {
    ibv_dereg_mr(*it);
  }
  rdma_destroy_ep(m_cm_id);
}

void Socket::register_memory(void* buffer, size_t size) {
  ibv_mr* mr = ibv_reg_mr(m_cm_id->pd, buffer, size, IBV_ACCESS_LOCAL_WRITE);
  if (!mr) {
    throw exception::socket::generic_error(
        "Error on ibv_reg_mr: " + std::string(strerror(errno)));
  }
  m_mrs.push_back(mr);
}

std::vector<iovec> Socket::poll_completed_send() {
  auto& wcs = m_comp_send;
  int ret = ibv_poll_cq(m_cm_id->send_cq, wcs.size(), &wcs.front());
  if (ret < 0) {
    throw exception::socket::generic_error(
        "Error on ibv_poll_cq: " + std::string(strerror(ret)));
  }
  std::vector<iovec> iov_vect;
  iov_vect.reserve(ret);
  for (auto wcs_it = std::begin(wcs); wcs_it != std::begin(wcs) + ret;
      ++wcs_it) {
    if (wcs_it->status) {
      throw exception::socket::generic_error(
          "Error status in wc of send_cq: "
              + std::string(ibv_wc_status_str(wcs_it->status)));
    }
    auto map_it = m_pending_send.find(reinterpret_cast<void*>(wcs_it->wr_id));
    if (map_it == std::end(m_pending_send)) {
      throw exception::socket::generic_error(
          "Error on erase: key element not exists");
    }
    iov_vect.push_back( { map_it->first, map_it->second });
    m_pending_send.erase(map_it);
  }

  return iov_vect;
}

std::vector<iovec> Socket::poll_completed_recv() {
  auto& wcs = m_comp_recv;
  int ret = ibv_poll_cq(m_cm_id->recv_cq, wcs.size(), &wcs.front());
  if (ret < 0) {
    throw exception::socket::generic_error(
        "Error on ibv_poll_cq: " + std::string(strerror(ret)));
  }
  std::vector<iovec> iov_vect;
  iov_vect.reserve(ret);
  for (auto wcs_it = std::begin(wcs); wcs_it != std::begin(wcs) + ret;
      ++wcs_it) {
    if (wcs_it->status) {
      throw exception::socket::generic_error(
          "Error status in wc of recv_cq: "
              + std::string(ibv_wc_status_str(wcs_it->status)));
    }
    auto map_it = m_pending_recv.find(reinterpret_cast<void*>(wcs_it->wr_id));
    if (map_it == std::end(m_pending_recv)) {
      throw exception::socket::generic_error(
          "Error on erase: key element not exists");
    }
    iov_vect.push_back( { map_it->first, map_it->second });
    m_pending_recv.erase(map_it);
  }

  return iov_vect;
}

void Socket::post_send(iovec const& iov) {

  if (!available_send()) {
    throw exception::socket::generic_error(
        "Error on post_send: no credits available");
  }

  auto mr_it = find_mr(m_mrs, iov);
  if (mr_it == std::end(m_mrs)) {
    throw exception::socket::generic_error(
        "Error on find_mr: no valid memory regions found");
  }

  ibv_sge sge;
  sge.addr = reinterpret_cast<uint64_t>(iov.iov_base);
  sge.length = iov.iov_len;
  sge.lkey = (*mr_it)->lkey;

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
    throw exception::socket::generic_error(
        "Error on ibv_post_send: " + std::string(strerror(ret)));
  }

  auto p = m_pending_send.insert(
      std::pair<void*, size_t>(iov.iov_base, iov.iov_len));
  if (!p.second) {
    throw exception::socket::generic_error(
        "Error on insert: key element already exists");
  }
}

void Socket::post_recv(iovec const& iov) {
  if (!available_recv()) {
    throw exception::socket::generic_error(
        "Error on post_recv: no credits available");
  }

  auto mr_it = find_mr(m_mrs, iov);
  if (mr_it == std::end(m_mrs)) {
    throw exception::socket::generic_error(
        "Error on find_mr: no valid memory regions found");
  }

  ibv_sge sge;
  sge.addr = reinterpret_cast<uint64_t>(iov.iov_base);
  sge.length = iov.iov_len;
  sge.lkey = (*mr_it)->lkey;

  ibv_recv_wr wr;
  wr.wr_id = reinterpret_cast<uint64_t>(iov.iov_base);
  wr.next = nullptr;
  wr.sg_list = &sge;
  wr.num_sge = 1;

  ibv_recv_wr* bad_wr;
  int ret = ibv_post_recv(m_cm_id->qp, &wr, &bad_wr);
  if (ret) {
    throw exception::socket::generic_error(
        "Error on ibv_post_recv: " + std::string(strerror(ret)));
  }

  auto p = m_pending_recv.insert(
      std::pair<void*, size_t>(iov.iov_base, iov.iov_len));
  if (!p.second) {
    throw exception::socket::generic_error(
        "Error on insert: key element already exists");
  }
}

bool Socket::available_send() {
  return m_pending_send.size() < m_credits;
}

bool Socket::available_recv() {
  return m_pending_recv.size() < m_credits;
}

std::vector<iovec> Socket::pending_send() {
  std::vector<iovec> iov_vect;
  iov_vect.reserve(m_pending_send.size());
  for (auto const& pair : m_pending_send) {
    iov_vect.push_back( { pair.first, pair.second });
  }
  return iov_vect;
}

std::vector<iovec> Socket::pending_recv() {
  std::vector<iovec> iov_vect;
  iov_vect.reserve(m_pending_recv.size());
  for (auto const& pair : m_pending_recv) {
    iov_vect.push_back( { pair.first, pair.second });
  }
  return iov_vect;
}

std::string Socket::peer_hostname() {
  char str[INET_ADDRSTRLEN];
  auto addr = reinterpret_cast<sockaddr_in*>(&m_cm_id->route.addr.dst_addr);
  inet_ntop(AF_INET, &(addr->sin_addr), str, INET_ADDRSTRLEN);
  return str;
}

}
