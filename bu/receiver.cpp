#include "bu/receiver.h"

#include <list>

#include "common/log.h"

namespace lseb {

Receiver::Receiver(std::vector<BuConnectionId> const& connection_ids)
    :
      m_connection_ids(connection_ids),
      m_bandwith(1.0) {

  // Registration
  for (auto& conn : m_connection_ids) {
    lseb_sync(conn);
  }

}

size_t Receiver::receive() {

  m_recv_timer.start();

  // Create a list of iterators and  read from all RUs
  std::list<std::vector<BuConnectionId>::iterator> conn_iterators;
  for (auto it = m_connection_ids.begin(); it != m_connection_ids.end(); ++it) {
    conn_iterators.push_back(it);
  }
  size_t bytes_read = 0;
  auto it = std::begin(conn_iterators);
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
      check_event_id = current_event_id + 1;
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

  m_recv_timer.pause();

  m_bandwith.add(bytes_read);
  if (m_bandwith.check()) {
    LOG(INFO)
      << "Bandwith: "
      << m_bandwith.frequency() / std::giga::num * 8.
      << " Gb/s";

    LOG(INFO) << "lseb_read() time: " << m_read_timer.rate() << "%";
    LOG(INFO) << "Receiver::receive() time: " << m_recv_timer.rate() << "%";
    m_read_timer.reset();
    m_recv_timer.reset();
  }
  return bytes_read;
}

}
