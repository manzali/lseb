#ifndef RU_SPLITTER_H
#define RU_SPLITTER_H

#include <vector>
#include <map>

#include "common/dataformat.h"

namespace lseb {

class Splitter {
  int m_id;
  int m_connections;
  int m_next_connection;
  DataRange m_data_range;

 public:
  Splitter(int id, int connections, DataRange const& data_range);
  std::map<int, std::vector<DataIov> > split(MultiEvents const& multievents);
};

}

#endif
