#include "bu/receiver.h"

#include <list>
#include <chrono>

#include "common/log.hpp"

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

bool Receiver::checkData(std::vector<iovec> total_iov) {

  // Check all data
  for (auto& i : total_iov) {
    size_t bytes_parsed = 0;
    uint64_t expected_event_id = pointer_cast<EventHeader>(i.iov_base)->id;
    bool warning = false;
    while (bytes_parsed < i.iov_len) {
      uint64_t current_event_id = pointer_cast<EventHeader>(
        static_cast<char*>(i.iov_base) + bytes_parsed)->id;
      uint64_t current_event_length = pointer_cast<EventHeader>(
        static_cast<char*>(i.iov_base) + bytes_parsed)->length;
      uint64_t current_event_flags = pointer_cast<EventHeader>(
        static_cast<char*>(i.iov_base) + bytes_parsed)->flags;

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
          return false;

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

  return true;;
}

size_t Receiver::receive(double ms_timeout) {

  // Create a list of iterators
  std::list<std::vector<BuConnectionId>::iterator> conn_iterators;
  for (auto it = m_connection_ids.begin(); it != m_connection_ids.end(); ++it) {
    conn_iterators.push_back(it);
  }

  std::vector<iovec> total_iov;

  auto start_time = std::chrono::high_resolution_clock::now();

  // Read from all RUs
  size_t bytes_read = 0;
  auto it = select_randomly(
    std::begin(conn_iterators),
    std::end(conn_iterators));

  while (it != std::end(conn_iterators) && std::chrono::duration<double,
      std::milli>(std::chrono::high_resolution_clock::now() - start_time).count() < ms_timeout) {
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
  checkData(total_iov);

  // Release data
  for (auto& conn : m_connection_ids) {
    lseb_release(conn);
  }

  // Warning missing data
  if (conn_iterators.size()) {
    LOG(WARNING)
      << "Missing data from "
      << conn_iterators.size()
      << " connections";
  }

  return bytes_read;
}

size_t Receiver::receiveAndForget() {
  size_t bytes_read = 0;
  for (auto& conn : m_connection_ids) {
    if (lseb_poll(conn)) {
      std::vector<iovec> iov = lseb_read(conn);
      for (auto& i : iov) {
        bytes_read += i.iov_len;
      }
      lseb_release(conn);
    }
  }
  return bytes_read;
}

}
