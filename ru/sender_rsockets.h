#ifndef RU_SENDER_H
#define RU_SENDER_H

#include "common/dataformat.h"

namespace lseb {

class Sender {
  MetaDataRange m_metadata_range;
  DataRange m_data_range;
  std::vector<int> m_connection_ids;
  int m_bu_id;

 public:
  Sender(
    MetaDataRange const& metadata_range,
    DataRange const& data_range,
    std::vector<int> const& connection_ids);
  size_t send(MultiEvents multievents);
};

}

#endif
