#ifndef RU_ACCUMULATOR_H
#define RU_ACCUMULATOR_H

#include "common/dataformat.h"

namespace lseb {

class Accumulator {
  MetaDataRange m_metadata_range;
  MetaDataRange::iterator m_current_metadata;
  DataRange m_data_range;
  DataRange::iterator m_current_data;
  unsigned int m_events_in_multievent;
  unsigned int m_generated_events;

 public:
  Accumulator(
    MetaDataRange const& metadata_range,
    DataRange const& data_range,
    unsigned int events_in_multievent);
  unsigned int add(MetaDataRange const& metadata_range);
  std::vector<MultiEvent> get_multievents(int n);

};

}

#endif
