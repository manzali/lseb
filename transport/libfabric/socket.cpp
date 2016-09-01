#include "transport/libfabric/socket.h"

#include <algorithm>
#include <arpa/inet.h>
#include <rdma/fi_cm.h>
#include <rdma/fi_errno.h>
#include "common/exception.h"
#include "domain.h"

namespace {
bool is_in_mr(const iovec &iov, const lseb::Socket::memory_region &mr) {
  bool res = mr.second.first <= iov.iov_base
      && static_cast<char *>(mr.second.first) + mr.second.second
          >= static_cast<char *>(iov.iov_base) + iov.iov_len;
  return res;
}
}

namespace lseb {

Socket::Socket(fid_ep *ep, fid_cq *rx_cq,
                   fid_cq *tx_cq, uint32_t credits)
    : m_ep(ep), m_rx_cq(rx_cq), m_tx_cq(tx_cq), m_credits(credits) {
}

void Socket::register_memory(void *buffer, size_t size) {
  fid_mr *mr;
  auto ret = fi_mr_reg(Domain::get_instance().get_raw_domain(),
                       buffer,
                       size,
                       FI_SEND | FI_RECV,
                       0 /* offset, must be 0 */,
                       0 /* requested key */,
                       0 /* flags, must be 0 */,
                       &mr,
                       0 /* context */);

  if (ret) {
    throw exception::socket::generic_error(
        "Error on fi_mr_reg: "
            + std::string(fi_strerror(static_cast<int>(-ret))));
  }

  m_mrs.emplace_back(mr_ptr(mr), mr_info(buffer,size));
}

std::vector<iovec> Socket::poll_completed_send() {
  std::vector<iovec> iov_vect;
  std::vector<fi_cq_entry> wcs(m_credits);

  auto ret = fi_cq_read(m_tx_cq.get(), wcs.data(), wcs.size());

  fi_cq_err_entry cq_err;
  const char *err = nullptr;

  if (ret < 0) {
    switch (ret) {
    case -FI_EAGAIN:
      return iov_vect;
    case -FI_EAVAIL:
      fi_cq_readerr(m_tx_cq.get(), &cq_err, 0 /* flags not documented */);
      err = fi_cq_strerror(m_tx_cq.get(),
                           cq_err.err,
                           cq_err.err_data,
                           nullptr,
                           0);
      break;
    default:
      // Seams to be impossible to reach this label, maybe remove it
      err = fi_strerror(static_cast<int>(-ret));
    }
    throw exception::socket::generic_error(
        "Error on fi_cq_read: " + std::string(err));
  }

  iov_vect.reserve(static_cast<decltype(iov_vect)::size_type > (ret));

  // Note: Is safe to store the map_end iterator because the erase function
  //       invalidates only references and iterators to the erased element
  auto map_end = std::end(m_pending_send);
  for (auto it = std::begin(wcs), end = it + ret; it!=end; ++it) {
    auto map_it = m_pending_send.find(it->op_context);
    if (map_it==map_end) {
      throw exception::socket::generic_error(
          "Error on find: key element not exists");
    }
    iov_vect.push_back({map_it->first, map_it->second});
    m_pending_send.erase(map_it);
  }

  return iov_vect;
}

std::vector<iovec> Socket::poll_completed_recv() {
  std::vector<fi_cq_entry> wcs(m_credits);

  auto ret = fi_cq_read(m_rx_cq.get(), wcs.data(), wcs.size());

  std::vector<iovec> iov_vect{};
  fi_cq_err_entry cq_err;
  const char *err = nullptr;

  if (ret < 0) {
    switch (ret) {
    case -FI_EAGAIN:
      return iov_vect;
    case -FI_EAVAIL:
      fi_cq_readerr(m_rx_cq.get(), &cq_err, 0 /* flags not documented */);
      err = fi_cq_strerror(m_rx_cq.get(),
                           cq_err.err,
                           cq_err.err_data,
                           nullptr,
                           0);
      break;
    default:
      // Seams to be impossible to reach this label, maybe remove it
      err = fi_strerror(static_cast<int>(-ret));
    }
    throw exception::socket::generic_error(
        "Error on fi_cq_read: " + std::string(err));
  }

  iov_vect.reserve(static_cast<decltype(iov_vect)::size_type > (ret));

  // Note: Is safe to store the map_end iterator because the erase function
  //       invalidates only references and iterators to the erased element
  auto map_end = std::end(m_pending_recv);
  for (auto it = std::begin(wcs), end = it + ret; it!=end; ++it) {
    auto map_it = m_pending_recv.find(it->op_context);
    if (map_it==map_end) {
      throw exception::socket::generic_error(
          "Error on find: key element not exists");
    }
    iov_vect.push_back({map_it->first, map_it->second});
    m_pending_recv.erase(map_it);
  }

  return iov_vect;
}

void Socket::post_send(iovec const &iov) {
  if (!available_send()) {
    throw exception::socket::generic_error(
        "Error on post_send: no credits available");
  }

  auto mr_it = std::find_if(std::begin(m_mrs),
                            std::end(m_mrs),
                            [&iov](decltype(m_mrs)
                                   ::const_reference m) -> bool {
                              return is_in_mr(iov, m);
                            });

  if (mr_it==std::end(m_mrs)) {
    throw exception::socket::generic_error(
        "Error on find_if: no valid memory region found");
  }

  auto ret = fi_send(m_ep.get(),
                     iov.iov_base,
                     iov.iov_len,
                     fi_mr_desc(mr_it->first.get()),
                     0, /* dest_address */
                     iov.iov_base);/* context */
  if (ret) {
    throw exception::socket::generic_error(
        "Error on fi_recv: "
            + std::string(fi_strerror(static_cast<int>(-ret))));
  }

  auto p = m_pending_send.emplace(iov.iov_base, iov.iov_len);
  if (!p.second) {
    throw exception::socket::generic_error(
        "Error on insert: key element already exists");
  }
}

void Socket::post_recv(iovec const &iov) {
  if (!available_recv()) {
    throw exception::socket::generic_error(
        "Error on post_recv: no credits available");
  }

  auto mr_it = std::find_if(std::begin(m_mrs),
                            std::end(m_mrs),
                            [&iov](decltype(m_mrs)
                                   ::const_reference m) -> bool {
                              return is_in_mr(iov, m);
                            });

  if (mr_it==std::end(m_mrs)) {
    throw exception::socket::generic_error(
        "Error on find_if: no valid memory region found");
  }

  auto ret = fi_recv(m_ep.get(),
                     iov.iov_base,
                     iov.iov_len,
                     fi_mr_desc(mr_it->first.get()),
                     0, /* src_address */
                     iov.iov_base);/* context */
  if (ret) {
    throw exception::socket::generic_error(
        "Error on fi_recv: "
            + std::string(fi_strerror(static_cast<int>(-ret))));
  }

  auto p = m_pending_recv.emplace(iov.iov_base, iov.iov_len);
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
  for (auto const &pair : m_pending_send) {
    iov_vect.push_back({pair.first, pair.second});
  }
  return iov_vect;
}

std::vector<iovec> Socket::pending_recv() {
  std::vector<iovec> iov_vect;
  for (auto const &pair : m_pending_recv) {
    iov_vect.push_back({pair.first, pair.second});
  }
  return iov_vect;
}

std::string Socket::peer_hostname() {
  char str[INET_ADDRSTRLEN];
  size_t len = sizeof(sockaddr_in);
  sockaddr_in addr[1];
  fi_getpeer(m_ep.get(), reinterpret_cast<void *>(addr), &len);
  inet_ntop(AF_INET, &(addr->sin_addr), str, INET_ADDRSTRLEN);
  return str;
}

}