#include "common/log.hpp"
#include "common/utility.h"

#include "ru/splitter.h"

namespace lseb {

Splitter::Splitter(
  std::vector<RuConnectionId> const& connection_ids,
  DataRange const& data_range)
    :
      m_connection_ids(connection_ids),
      m_next_bu(std::begin(m_connection_ids)),
      m_data_range(data_range) {
}

std::map<int, std::vector<DataIov> > Splitter::split(
  MultiEvents const& multievents) {
  std::map<int, std::vector<DataIov> > iov_map;
  for (auto const& multievent : multievents) {
    iov_map[m_next_bu - std::begin(m_connection_ids)].push_back(
      create_iovec(multievent.second, m_data_range));
    if (++m_next_bu == std::end(m_connection_ids)) {
      m_next_bu = std::begin(m_connection_ids);
    }
  }

  return iov_map;
}

}
