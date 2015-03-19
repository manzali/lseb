#ifndef RU_SENDER_H
#define RU_SENDER_H

#include <random>

#include "common/dataformat.h"

namespace lseb {

using MultiEvents = std::vector<std::pair<MetaDataRange, DataRange> >;

class Sender {
  MetaDataRange m_metadata_range;
  MetaDataRange::iterator m_current_metadata;
  DataRange m_data_range;
  DataRange::iterator m_current_data;
  std::vector<int> m_connection_ids;
  size_t m_multievent_size;
  std::mt19937 m_rand;
  unsigned int m_generated_events;
  int m_bu_id;

 public:
  Sender(MetaDataRange const& metadata_range, DataRange const& data_range,
         std::vector<int> const& connection_ids, size_t multievent_size);
  MultiEvents add(MetaDataRange metadata_range);
  size_t send(MultiEvents multievents);

};

}

#endif
