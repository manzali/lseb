#include "bu/receiver.h"

#include <list>

#include "common/log.hpp"
#include "common/utility.h"

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

std::vector<iovec> Receiver::receive(std::chrono::milliseconds ms_timeout) {

  // Create a list of iterators
  std::list<std::vector<BuConnectionId>::iterator> conn_iterators;
  for (auto it = m_connection_ids.begin(); it != m_connection_ids.end(); ++it) {
    conn_iterators.push_back(it);
  }

  std::vector<iovec> total_iov;

  // Read from all RUs
  auto it = select_randomly(
    std::begin(conn_iterators),
    std::end(conn_iterators));

  std::chrono::high_resolution_clock::time_point end_time =
    std::chrono::high_resolution_clock::now() + ms_timeout;

  while (it != std::end(conn_iterators) && std::chrono::high_resolution_clock::now() < end_time) {
    if (lseb_poll(**it)) {
      std::vector<iovec> conn_iov = lseb_read(**it);
      for (auto& i : conn_iov) {
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

  // Warning missing data
  if (conn_iterators.size()) {
    LOG(WARNING)
      << "Missing data from "
      << conn_iterators.size()
      << " connections";
  }

  return total_iov;
}

void Receiver::release(){
  // Release data
  for (auto& conn : m_connection_ids) {
    lseb_release(conn);
  }
}

}
