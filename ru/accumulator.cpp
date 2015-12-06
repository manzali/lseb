#include "ru/accumulator.h"

#include <algorithm>

#include "common/log.hpp"
#include "common/utility.h"

namespace lseb {

Accumulator::Accumulator(
  Controller const& controller,
  MetaDataRange const& metadata_range,
  DataRange const& data_range,
  int events_in_multievent)
    :
      m_controller(controller),
      m_metadata_range(metadata_range),
      m_data_range(data_range),
      m_current_metadata(std::begin(m_metadata_range)),
      m_events_in_multievent(events_in_multievent),
      m_generated_events(0),
      m_release_metadata(std::begin(m_metadata_range)) {
}

bool Accumulator::checkDataWrap(MetaDataRange multievent_metadata){
  uint64_t first_offset = std::begin(multievent_metadata)->offset;
  auto it =
    find_in_range(
      multievent_metadata,
      m_metadata_range,
      [first_offset](EventMetaData const& m) {return (m.offset < first_offset) ? true : false;});
  return it != std::end(multievent_metadata);
}

std::pair<iovec, bool> Accumulator::get_multievent() {

  // If not enough data ready, read data from the Controller
  if (m_generated_events < m_events_in_multievent) {
    MetaDataRange meta = m_controller.read();
    m_generated_events += distance_in_range(meta, m_metadata_range);
  }

  std::pair<iovec, bool> p;

  if (m_generated_events >= m_events_in_multievent) {

  // Create bulked metadata and data ranges
  MetaDataRange multievent_metadata(
    m_current_metadata,
    advance_in_range(
      m_current_metadata,
      m_events_in_multievent,
      m_metadata_range));

  // Check for wrap
  bool wrap = checkDataWrap(multievent_metadata);
  assert(wrap == 0 && "Wrap found!");

  // Find data begin
  auto data_begin = std::begin(m_data_range) + multievent_metadata.begin()
    ->offset;

  // Find data end
  auto last_metadata = advance_in_range(
    std::end(multievent_metadata),
    -1,
    m_metadata_range);
  auto data_end =
    std::begin(m_data_range) + last_metadata->offset + last_metadata->length;

  p.first = { data_begin, (size_t) std::distance(data_begin, data_end) };
  m_iov_multievents.emplace_back(p.first.iov_base, false);
  p.second = true;

  m_generated_events -= m_events_in_multievent;
  m_current_metadata = std::end(multievent_metadata);
  }
  else {
    p.second = false;
  }

  return p;
}

void Accumulator::release_multievents(std::vector<void*> const& vect) {
// Update flag of m_iov_multievents
  for (auto& v : vect) {
    auto it =
      std::find_if(
        std::begin(m_iov_multievents),
        std::end(m_iov_multievents),
        [&v] (std::pair<void*, bool> const& p) {return p.first == v;});
    assert(it != std::end(m_iov_multievents));
    assert(it->second == false);
    it->second = true;
  }

// Release contiguous memory
  auto it = std::find_if(
    std::begin(m_iov_multievents),
    std::end(m_iov_multievents),
    [](std::pair<void*, bool> const& p) {return p.second == false;});
  int multievents_to_release = std::distance(std::begin(m_iov_multievents), it);
  assert(multievents_to_release >= 0);
  if (multievents_to_release) {
    MetaDataRange metadata_to_release(
      m_release_metadata,
      advance_in_range(
        m_release_metadata,
        m_events_in_multievent * multievents_to_release,
        m_metadata_range));
    m_release_metadata = std::end(metadata_to_release);
    m_controller.release(metadata_to_release);
    m_iov_multievents.erase(
      std::begin(m_iov_multievents),
      std::begin(m_iov_multievents) + multievents_to_release);
  }
}

}
