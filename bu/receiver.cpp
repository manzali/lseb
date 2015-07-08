#include "bu/receiver.h"

namespace lseb {

Receiver::Receiver(std::vector<BuConnectionId> const& connection_ids)
    :
      m_connection_ids(connection_ids) {
}

std::map<int, std::vector<iovec> > Receiver::receive() {
  std::map<int, std::vector<iovec> > iov_map;
  for (int conn = 0; conn < m_connection_ids.size(); ++conn) {
    std::vector<iovec> conn_iov = lseb_read(m_connection_ids[conn]);
    if (conn_iov.size()) {
      iov_map[conn] = conn_iov;
    }
  }
  return iov_map;
}

void Receiver::release(std::map<int, std::vector<void*> > const& wrs_map) {
  for (auto const& m : wrs_map) {
    lseb_release(m_connection_ids[m.first], m.second);
  }
}

}
