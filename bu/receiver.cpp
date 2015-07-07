#include "bu/receiver.h"

#include "common/log.hpp"
#include "common/utility.h"

namespace lseb {

Receiver::Receiver(std::vector<BuConnectionId> const& connection_ids)
    :
      m_connection_ids(connection_ids) {
}

std::map<int, std::vector<iovec> > Receiver::receive(
  std::chrono::milliseconds ms_timeout) {
  std::map<int, std::vector<iovec> > iov_map;
  // Due to non blocking call, the timeout is ignored (verbs version)
  for (auto it = std::begin(m_connection_ids); it != std::end(m_connection_ids);
      ++it) {
    std::vector<iovec> conn_iov = lseb_read(*it);
    if (conn_iov.size()) {
      iov_map.emplace(it - std::begin(m_connection_ids), std::move(conn_iov));
    }
  }
  return iov_map;
}

void Receiver::release(std::map<int, std::vector<void*> > const& wrs_map) {
  for (auto const& m : wrs_map) {
    lseb_relase(m_connection_ids[m.first], m.second);
  }
}

}
