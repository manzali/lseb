#include "bu/receiver.h"

namespace lseb {

Receiver::Receiver(
  size_t events_in_multievent,
  std::vector<BuConnectionId> const& connection_ids)
    :
      m_events_in_multievent(events_in_multievent),
      m_connection_ids(connection_ids) {

  // Registration
  for (auto& conn : m_connection_ids) {
    lseb_register(conn);
  }

}

size_t Receiver::receive() {

  // Create a vector of iterators and  wait until all rus have written
  std::vector<std::vector<BuConnectionId>::iterator> conn_iterators;
  for (auto it = m_connection_ids.begin(); it != m_connection_ids.end(); ++it) {
    if(!lseb_poll(*it)){
      conn_iterators.emplace_back(it);
    }
  }
  auto it = std::begin(conn_iterators);
  while (it != std::end(conn_iterators)) {
    if (lseb_poll(**it)) {
      it = conn_iterators.erase(it);
    } else {
      ++it;
    }
    if(it == std::end(conn_iterators)){
      it = std::begin(conn_iterators);
    }
  }

  // Read all data
  size_t read_bytes = 0;
  for (auto& conn : m_connection_ids) {
    ssize_t ret = lseb_read(conn, m_events_in_multievent);
    assert(ret != -1);
    read_bytes += ret;
  }
  return read_bytes;
}

}
