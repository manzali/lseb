#include "bu/receiver.h"

#include <list>

#include "common/log.h"

namespace lseb {

Receiver::Receiver(std::vector<BuConnectionId> const& connection_ids)
    :
      m_connection_ids(connection_ids) {
  LOG(INFO) << "Waiting for synchronization...";
  for (auto& conn : m_connection_ids) {
    lseb_sync(conn);
  }
  LOG(INFO) << "Synchronization completed";

}

size_t Receiver::receive() {

  // Create a list of iterators
  std::list<std::vector<BuConnectionId>::iterator> conn_iterators;
  for (auto it = m_connection_ids.begin(); it != m_connection_ids.end(); ++it) {
    conn_iterators.push_back(it);
  }

  // Read from all RUs
  size_t bytes_read = 0;
  auto it = select_randomly(
    std::begin(conn_iterators),
    std::end(conn_iterators));
  while (it != std::end(conn_iterators)) {
    if (lseb_poll(**it)) {
      ssize_t ret = lseb_read(**it);
      assert(ret != -1);
      bytes_read += ret;
      it = conn_iterators.erase(it);
    } else {
      ++it;
    }
    if (it == std::end(conn_iterators)) {
      it = std::begin(conn_iterators);
    }
  }

  // Check all data
  for (auto& conn : m_connection_ids) {
    size_t len = conn.avail;
    size_t bytes_parsed = 0;
    uint64_t check_event_id = pointer_cast<EventHeader>(conn.buffer)->id;
    while (bytes_parsed < len) {
      uint64_t current_event_id = pointer_cast<EventHeader>(
        static_cast<char*>(conn.buffer) + bytes_parsed)->id;
      assert(check_event_id == current_event_id || current_event_id == 0);
      check_event_id = ++current_event_id;
      bytes_parsed += pointer_cast<EventHeader>(
        static_cast<char*>(conn.buffer) + bytes_parsed)->length;

    }
    if (bytes_parsed > len) {
      LOG(WARNING) << "Wrong length read";
    }
  }

  // Release all data
  for (auto& conn : m_connection_ids) {
    lseb_release(conn);
  }

  return bytes_read;
}

size_t Receiver::receive_and_forget() {
  size_t bytes_read = 0;
  for (auto& conn : m_connection_ids) {
    if(lseb_poll(conn)){
      ssize_t ret = lseb_read(conn);
      assert(ret != -1);
      bytes_read += ret;
      lseb_release(conn);
    }
  }
  return bytes_read;
}

}
