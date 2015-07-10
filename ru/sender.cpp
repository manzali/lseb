#include "common/log.hpp"
#include "common/utility.h"

#include "ru/sender.h"

namespace lseb {

Sender::Sender(std::vector<RuConnectionId> const& connection_ids)
    :
      m_connection_ids(connection_ids) {
}

size_t Sender::send(
  std::map<int, std::vector<DataIov> > const& iov_map,
  std::chrono::milliseconds const& ms_timeout) {

  std::chrono::high_resolution_clock::time_point const end_time =
    std::chrono::high_resolution_clock::now() + ms_timeout;

  size_t bytes_sent = 0;

  // Creste the new map with iterators
  std::map<int,
      std::pair<std::vector<DataIov>::const_iterator,
          std::vector<DataIov>::const_iterator> > it_map;
  for (auto const& m : iov_map) {
    it_map[m.first] = std::make_pair(std::begin(m.second), std::end(m.second));
  }

  // Iterate until the map is empty (or timeout is reached)
  auto it = std::begin(it_map);
  while (it != std::end(it_map) && std::chrono::high_resolution_clock::now() < end_time) {
    bool remove_it = false;
    int avail = lseb_avail(m_connection_ids[it->first]);
    if (avail) {

      if (avail >= it->second.second - it->second.first) {
        avail = it->second.second - it->second.first;
        remove_it = true;
      }

      std::vector<DataIov> subvect(it->second.first, it->second.first + avail);
      size_t load = 0;
      for (auto const& el : subvect) {
        load += iovec_length(el);
      }

      LOG(DEBUG) << "Writing " << subvect.size() << " wr (" << it->second.second - (it
        ->second.first + avail) << " remaining) to connection " << it->first;

      size_t ret = lseb_write(m_connection_ids[it->first], subvect);
      assert(ret == load);
      bytes_sent += ret;

      it->second.first += avail;

    }
    if (remove_it) {
      it = it_map.erase(it);
    } else {
      ++it;
    }

    if (it == std::end(it_map)) {
      it = std::begin(it_map);
    }
  }

  if (it_map.size()) {
    LOG(WARNING) << "Uncompleted send";
  }

  return bytes_sent;
}

}
