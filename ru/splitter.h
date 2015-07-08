#ifndef RU_SPLITTER_H
#define RU_SPLITTER_H

#include <vector>
#include <map>

#include "common/dataformat.h"
#include "transport/transport.h"

namespace lseb {

class Splitter {
  std::vector<RuConnectionId> m_connection_ids;
  std::vector<RuConnectionId>::iterator m_next_bu;
  DataRange m_data_range;

 public:
  Splitter(
    std::vector<RuConnectionId> const& connection_ids,
    DataRange const& data_range);
  std::map<int, std::vector<DataIov> > split(MultiEvents const& multievents);
};

}

#endif
