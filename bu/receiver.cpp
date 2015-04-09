#include "bu/receiver.h"

#include <algorithm>

#include "common/log.h"

namespace lseb {

Receiver::Receiver(
  size_t events_in_multievent,
  std::vector<BuConnectionId> const& connection_ids)
    :
      m_events_in_multievent(events_in_multievent),
      m_connection_ids(connection_ids) {

  // Registration
  for(auto& id : m_connection_ids){
    lseb_register(id);
  }

}

size_t Receiver::receive() {
  size_t read_bytes = 0;
  for (auto& conn : m_connection_ids) {
    if (lseb_poll(conn)) {
      ssize_t ret = lseb_read(conn, m_events_in_multievent);
      assert(ret != -1);
      read_bytes += ret;
    }
  }
  return read_bytes;
}

}
