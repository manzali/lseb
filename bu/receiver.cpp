#include "bu/receiver.h"
#include "common/log.hpp"

namespace lseb {

Receiver::Receiver(std::vector<BuConnectionId> const& connection_ids)
    :
      m_connection_ids(connection_ids) {
}

std::map<int, std::vector<iovec> > Receiver::receive() {
  std::map<int, std::vector<iovec> > iov_map;
  for (int conn = 0; conn < m_connection_ids.size(); ++conn) {
    std::vector<iovec> conn_iov = receive(conn);
    if (conn_iov.size()) {
      iov_map[conn] = conn_iov;
    }
  }
  return iov_map;
}

std::vector<iovec> Receiver::receive(int conn) {
  std::vector<iovec> conn_iov;
  if (lseb_poll(m_connection_ids[conn])) {
    conn_iov = lseb_read(m_connection_ids[conn]);
  }
  return conn_iov;
}

void Receiver::release(std::vector<std::vector<void*> > const& wrs_vects) {
  assert(wrs_vects.size() <= m_connection_ids.size());
  for (int i = 0; i < wrs_vects.size(); ++i) {
    if (!wrs_vects[i].empty()) {
      LOG(DEBUG)
        << "Posting "
        << wrs_vects[i].size()
        << " credits to connection "
        << i;
      lseb_release(m_connection_ids[i], wrs_vects[i]);
    }
  }
}

}
