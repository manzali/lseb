#include "common/log.hpp"
#include "common/utility.h"

#include "ru/splitter.h"

namespace lseb {

Splitter::Splitter(int connections, DataRange const& data_range)
    :
      m_connections(connections),
      m_next_connection(0),
      m_data_range(data_range) {
}

std::map<int, std::vector<DataIov> > Splitter::split(
  MultiEvents const& multievents) {
  std::map<int, std::vector<DataIov> > iov_map;
  for (auto const& multievent : multievents) {
    iov_map[m_next_connection].push_back(
      create_iovec(multievent.second, m_data_range));
    if (++m_next_connection == m_connections) {
      m_next_connection = 0;
    }
  }

  return iov_map;
}

}
