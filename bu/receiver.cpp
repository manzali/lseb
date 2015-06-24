#include "bu/receiver.h"

#include <list>
#include <chrono>

#include "common/log.hpp"

namespace lseb {

void checkData(iovec const& iov) {
  size_t bytes_parsed = 0;
  uint64_t expected_event_id = pointer_cast<EventHeader>(iov.iov_base)->id;
  bool warning = false;
  while (bytes_parsed < iov.iov_len) {
    uint64_t current_event_id = pointer_cast<EventHeader>(
      static_cast<char*>(iov.iov_base) + bytes_parsed)->id;
    uint64_t current_event_length = pointer_cast<EventHeader>(
      static_cast<char*>(iov.iov_base) + bytes_parsed)->length;
    uint64_t current_event_flags = pointer_cast<EventHeader>(
      static_cast<char*>(iov.iov_base) + bytes_parsed)->flags;

    if (expected_event_id != current_event_id) {
      if (warning) {
        // Print event header
        LOG(WARNING)
          << "Error parsing EventHeader:"
          << std::endl
          << "expected event id: "
          << expected_event_id
          << std::endl
          << "event id: "
          << current_event_id
          << std::endl
          << "event length: "
          << current_event_length
          << std::endl
          << "event flags: "
          << current_event_flags;

        // terminate parsing
        return;

      } else {
        // if the event id is different from the expected one, check the next
        warning = true;
      }
    } else {
      warning = false;
    }
    expected_event_id = ++current_event_id;
    bytes_parsed += current_event_length;
  }
}

Receiver::Receiver(std::vector<BuConnectionId> const& connection_ids)
    :
      m_connection_ids(connection_ids),
      m_executor(2) {
  LOG(INFO) << "Waiting for synchronization...";
  for (auto& conn : m_connection_ids) {
    lseb_sync(conn);
  }
  LOG(INFO) << "Synchronization completed";
}

size_t Receiver::receive(std::chrono::milliseconds ms_timeout) {

  // Create a list of iterators
  std::list<std::vector<BuConnectionId>::iterator> conn_iterators;
  for (auto it = m_connection_ids.begin(); it != m_connection_ids.end(); ++it) {
    conn_iterators.push_back(it);
  }

  std::vector<iovec> total_iov;

  // Read from all RUs
  size_t bytes_read = 0;
  auto it = select_randomly(
    std::begin(conn_iterators),
    std::end(conn_iterators));

  std::chrono::high_resolution_clock::time_point end_time =
    std::chrono::high_resolution_clock::now() + ms_timeout;

  while (it != std::end(conn_iterators) && std::chrono::high_resolution_clock::now() < end_time) {
    if (lseb_poll(**it)) {
      std::vector<iovec> conn_iov = lseb_read(**it);
      for (auto& i : conn_iov) {
        bytes_read += i.iov_len;
        total_iov.push_back(i);
      }
      it = conn_iterators.erase(it);
    } else {
      ++it;
    }
    if (it == std::end(conn_iterators)) {
      it = std::begin(conn_iterators);
    }
  }

  // Check data
  for (auto& iov : total_iov) {
    m_executor.post(std::bind(checkData, iov));
  }
  m_executor.wait();

  // Release data
  for (auto& conn : m_connection_ids) {
    lseb_release(conn);
    m_executor.post(std::bind(lseb_release, conn));
  }
  m_executor.wait();

  // Warning missing data
  if (conn_iterators.size()) {
    LOG(WARNING)
      << "Missing data from "
      << conn_iterators.size()
      << " connections";
  }

  return bytes_read;
}

}
