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

void Receiver::release(std::map<int, std::vector<iovec> > const& wrs_map) {
  assert(wrs_map.size() <= m_connection_ids.size());
  for (auto const& wrs_pair : wrs_map) {
    LOG(DEBUG)
      << "Posting "
      << wrs_pair.second.size()
      << " credits to connection "
      << wrs_pair.first;
    lseb_release(m_connection_ids[wrs_pair.first], wrs_pair.second);
  }
}

}
