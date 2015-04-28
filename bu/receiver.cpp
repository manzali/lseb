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
    lseb_register(conn);
  }

}

size_t Receiver::receive() {

  m_recv_timer.start();

  // Create a list of iterators and  wait until all rus have written
  std::list<std::vector<BuConnectionId>::iterator> conn_iterators;
  for (auto it = m_connection_ids.begin(); it != m_connection_ids.end(); ++it) {
    if (!lseb_poll(*it)) {
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
    if (it == std::end(conn_iterators)) {
      it = std::begin(conn_iterators);
    }
  }

  // Read all data
  size_t read_bytes = 0;
  for (auto& conn : m_connection_ids) {
    m_read_timer.start();
    ssize_t ret = lseb_read(conn);
    m_read_timer.pause();
    assert(ret != -1);
    read_bytes += ret;
  }

  m_recv_timer.pause();

  m_bandwith.add(read_bytes);
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
  return read_bytes;
}

}
