#include "generator/generator.h"

#include <cmath>
#include <cassert>

#include "commons/pointer_cast.h"

namespace lseb {

static size_t const data_padding = 32;

template<size_t N>
size_t roundUpPowerOf2(size_t val) {
  static_assert((N & (N - 1)) == 0, "N must be a power of 2");
  return (val + N - 1) & ~(N - 1);
}

Generator::Generator(LengthGenerator const& payload_length_generator,
                     EventMetaData* begin_metadata, EventMetaData* end_metadata,
                     char* begin_data, char* end_data)
    : m_payload_length_generator(payload_length_generator),
      m_begin_metadata(begin_metadata),
      m_ring_metadata(std::distance(begin_metadata, end_metadata)),
      m_begin_data(begin_data),
      m_end_data(end_data),
      m_avail_data(std::distance(begin_data, end_data)),
      m_current_event_id(0) {
  assert(std::distance(begin_metadata, end_metadata) > 0);
  assert(std::distance(m_begin_data, m_end_data) > 0);
  assert(data_padding >= sizeof(EventHeader));
  assert(std::distance(m_begin_data, m_end_data) % data_padding == 0);
}

void Generator::releaseEvents(size_t n_events) {
  assert(
      n_events <= m_ring_metadata.size()
          && "Wrong number of events to release");
  while (n_events != 0) {
    m_avail_data += m_ring_metadata.front()->length;
    m_ring_metadata.pop_front();
    --n_events;
  }
}

size_t Generator::generateEvents(size_t n_events) {
  // Set current data pointer (next to be written)
  char* current_data = m_begin_data;
  if (!m_ring_metadata.empty()) {
    EventMetaData const& last_metadata = *m_ring_metadata.back();
    size_t const data_capacity = std::distance(m_begin_data, m_end_data);
    assert(last_metadata.offset < data_capacity);
    size_t const offset = last_metadata.length + last_metadata.offset;
    // Handle wrap
    current_data += (offset >= data_capacity) ? offset - data_capacity : offset;
  }

  // Start creating events
  size_t i = 0;
  size_t const e =
      (n_events <= m_ring_metadata.capacity() - m_ring_metadata.size()) ?
          n_events : m_ring_metadata.capacity() - m_ring_metadata.size();

  size_t event_size = roundUpPowerOf2<data_padding>(
      sizeof(EventHeader) + m_payload_length_generator.generate());

  while (i != e && m_avail_data >= event_size) {

    // Set current metadata pointer (next to be written)
    EventMetaData* current_metadata = m_begin_metadata
        + (m_current_event_id % m_ring_metadata.capacity());

    ssize_t offset = std::distance(m_begin_data, current_data);
    assert(offset >= 0);

    // Set EventMetaData
    EventMetaData& metadata = *(new (current_metadata) EventMetaData(
        m_current_event_id, event_size, offset));

    // Set EventHeader
    EventHeader& header =
        *(new (pointer_cast<EventHeader>(current_data)) EventHeader(
            m_current_event_id, metadata.length));

    // Update ring metadata pointer
    m_ring_metadata.push_back(current_metadata);

    // Update current_data pointer
    size_t const dist_to_end_data = std::distance(current_data, m_end_data);
    current_data =
        (event_size < dist_to_end_data) ?
            current_data + event_size :
            m_begin_data + event_size - dist_to_end_data;

    // Update counters
    ++i;
    ++m_current_event_id;
    m_avail_data -= event_size;

    // Next event size
    event_size = roundUpPowerOf2<data_padding>(
        sizeof(header) + m_payload_length_generator.generate());

  }

  assert(n_events >= i && "Generated too much events");

  return i;
}

}
