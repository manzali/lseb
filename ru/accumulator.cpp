#include "ru/accumulator.h"

#include <algorithm>

#include "common/log.hpp"
#include "common/utility.h"

namespace lseb {

Accumulator::Accumulator(
  MetaDataRange const& metadata_range,
  DataRange const& data_range,
  unsigned int events_in_multievent)
    :
      m_metadata_range(metadata_range),
      m_current_metadata(std::begin(m_metadata_range)),
      m_data_range(data_range),
      m_current_data(std::begin(m_data_range)),
      m_events_in_multievent(events_in_multievent),
      m_generated_events(0) {
}

unsigned int Accumulator::add(MetaDataRange const& metadata_range) {
  m_generated_events += distance_in_range(metadata_range, m_metadata_range);
  return m_generated_events;
}

MultiEvents Accumulator::get_multievents(int n) {
  MultiEvents multievents;
  // Handle bulk submission and release events

  assert(m_generated_events >= m_events_in_multievent * n);

  for (int i = 0; i < n; ++i) {
    // Create bulked metadata and data ranges
    MetaDataRange multievent_metadata(
      m_current_metadata,
      advance_in_range(
        m_current_metadata,
        m_events_in_multievent,
        m_metadata_range));

    m_current_metadata = std::end(multievent_metadata);

    size_t const data_length =
      accumulate_in_range(
        multievent_metadata,
        m_metadata_range,
        size_t(0),
        [](size_t partial, EventMetaData const& it) {return partial + it.length;});

    DataRange multievent_data(
      m_current_data,
      advance_in_range(m_current_data, data_length, m_data_range));
    m_current_data = std::end(multievent_data);

    multievents.emplace_back(
      std::move(multievent_metadata),
      std::move(multievent_data));
    m_generated_events -= m_events_in_multievent;
  }
  return multievents;
}

}
