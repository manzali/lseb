#ifndef RU_SENDER_H
#define RU_SENDER_H

#include "common/dataformat.h"

#include "transport/transport.h"

namespace lseb {

class Sender {
  MetaDataRange m_metadata_range;
  DataRange m_data_range;
  std::vector<RuConnectionId> m_connection_ids;
  std::vector<RuConnectionId>::iterator m_next_bu;
  size_t m_max_sending_size;

 public:
  Sender(
    MetaDataRange const& metadata_range,
    DataRange const& data_range,
    std::vector<RuConnectionId> const& connection_ids,
    size_t max_sending_size);
  size_t send(MultiEvents multievents);
};

}

#endif
