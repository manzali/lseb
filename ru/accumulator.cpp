#include "ru/accumulator.h"

#include <algorithm>

#include "common/log.hpp"
#include "common/utility.h"

namespace lseb {

Accumulator::Accumulator(
  Controller const& controller,
  MetaDataRange const& metadata_range,
  DataRange const& data_range,
  int events_in_multievent,
  int required_multievents)
    :
      m_controller(controller),
      m_metadata_range(metadata_range),
      m_data_range(data_range),
      m_current_metadata(std::begin(m_metadata_range)),
      m_events_in_multievent(events_in_multievent),
      m_required_multievents(required_multievents),
      m_generated_events(0),
      m_release_metadata(std::begin(m_metadata_range)) {
}

std::vector<iovec> Accumulator::get_multievents() {

  MetaDataRange meta = m_controller.read();
  m_generated_events += distance_in_range(meta, m_metadata_range);

  std::vector<iovec> iov_vect;

  if (m_generated_events >= m_events_in_multievent * m_required_multievents) {

    for (int i = 0; i < m_required_multievents; ++i) {

      // Create bulked metadata and data ranges
      MetaDataRange multievent_metadata(
        m_current_metadata,
        advance_in_range(
          m_current_metadata,
          m_events_in_multievent,
          m_metadata_range));

      uint64_t first_offset = std::begin(multievent_metadata)->offset;

      auto it =
        find_in_range(
          multievent_metadata,
          m_metadata_range,
          [first_offset](EventMetaData const& m) {return (m.offset < first_offset) ? true : false;});

      assert(it == std::end(multievent_metadata) && "Wrap found");

      auto data_begin = std::begin(m_data_range) + multievent_metadata.begin()
        ->offset;

      auto last_metadata = advance_in_range(
        std::end(multievent_metadata),
        -1,
        m_metadata_range);

      auto data_end =
        std::begin(m_data_range) + last_metadata->offset + last_metadata->length;

      iov_vect.push_back(
        { data_begin, (size_t) std::distance(data_begin, data_end) });

      m_generated_events -= m_events_in_multievent;
      m_current_metadata = std::end(multievent_metadata);
    }

    // Fill m_iov_multievents
    for (auto& iov : iov_vect) {
      m_iov_multievents.emplace_back(iov.iov_base, false);
    }
  }

  return iov_vect;
}

void Accumulator::release_multievents(std::vector<iovec> const& iov_vect) {
// Update flag of m_iov_multievents
  for (auto& iov : iov_vect) {
    auto it =
      std::find_if(
        std::begin(m_iov_multievents),
        std::end(m_iov_multievents),
        [&iov] (std::pair<void*, bool> const& p) {return p.first == iov.iov_base;});
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
