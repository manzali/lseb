#include "generator/generator.h"

#include <cassert>

#include "commons/pointer_cast.h"

namespace lseb {

template<typename T, typename RandomAccessIterator>
bool enoughSpaceFor(RandomAccessIterator first, RandomAccessIterator last,
                    size_t payload_size = 0) {
  return std::distance(first, last) >= sizeof(T) + payload_size;
}

Generator::Generator(char* begin_data, SizeGenerator const& payload_size_generator)
    : m_begin_data(begin_data),
      m_current_event_id(0),
      m_payload_size_generator(payload_size_generator) {
}

uint64_t Generator::generateEvents(EventMetaData* current_metadata,
                                   EventMetaData* end_metadata,
                                   char* current_data, char* end_data) {

  size_t payload_size = m_payload_size_generator.generate();

  uint64_t const starting_event_id = m_current_event_id;

  while (current_metadata != end_metadata
      && enoughSpaceFor<EventHeader>(current_data, end_data, payload_size)) {

    assert(
        std::distance(m_begin_data, current_data) >= 0
            && "Checking data addresses");

    // Set EventMetaData
    EventMetaData& metadata = *(new (current_metadata) EventMetaData(
        m_current_event_id, sizeof(EventHeader) + payload_size,
        std::distance(m_begin_data, current_data)));

    // Set EventHeader
    EventHeader& header =
        *(new (pointer_cast<EventHeader>(current_data)) EventHeader(
            m_current_event_id, metadata.length));

    // Update offsets and increment counters
    ++current_metadata;
    current_data += header.length;
    ++m_current_event_id;

    // Compute new payload size
    payload_size = m_payload_size_generator.generate();
  }

  assert(
      m_current_event_id >= starting_event_id
          && "Checking overflow of event ID");

  return m_current_event_id - starting_event_id;
}

}
