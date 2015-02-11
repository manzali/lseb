#include "generator/generator.h"

#include <cassert>

#include "commons/pointer_cast.h"

namespace lseb {

namespace {
size_t const data_padding = 32;
}

template<size_t N>
size_t roundUpPowerOf2(size_t val) {
  static_assert((N & (N - 1)) == 0, "N must be a power of 2");
  return (val + N - 1) & ~(N - 1);
}

Generator::Generator(SizeGenerator const& payload_size_generator,
                     EventMetaData* begin_metadata, EventMetaData* end_metadata,
                     char* begin_data, char* end_data)
    : m_payload_size_generator(payload_size_generator),
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

size_t Generator::releaseEvents(size_t n_events) {
  size_t i = 0;
  size_t const e =
      (n_events <= m_ring_metadata.size()) ? n_events : m_ring_metadata.size();
  while (i != e) {
    m_avail_data += m_ring_metadata[0]->length;
    m_ring_metadata.pop_front();
    ++i;
  }
  return i;
}

size_t Generator::generateEvents(size_t n_events) {

  // Set current data pointer (next to be written)
  char* current_data = m_begin_data;
  if (!m_ring_metadata.empty()) {
    EventMetaData* last_metadata = m_ring_metadata.back();
    current_data += (last_metadata->offset + last_metadata->length);
  }

  // Start creating events
  size_t i = 0;
  size_t const e =
      (n_events <= m_ring_metadata.capacity() - m_ring_metadata.size()) ?
          n_events : m_ring_metadata.capacity() - m_ring_metadata.size();

  size_t event_size = roundUpPowerOf2<data_padding>(
      sizeof(EventHeader) + m_payload_size_generator.generate());

  while (i != e && m_avail_data >= event_size) {

    // Set current metadata pointer (next to be written)
    EventMetaData* current_metadata = m_begin_metadata
        + (m_current_event_id % m_ring_metadata.capacity());

    // Set EventMetaData
    EventMetaData& metadata = *(new (current_metadata) EventMetaData(
        m_current_event_id, event_size,
        std::distance(m_begin_data, current_data)));

    // Check current_data pointer wrap
    if (current_data == m_end_data) {
      current_data = m_begin_data;
    }

    // Set EventHeader
    EventHeader& header =
        *(new (pointer_cast<EventHeader>(current_data)) EventHeader(
            m_current_event_id, metadata.length));

    // Update ring metadata pointer
    m_ring_metadata.push_back(current_metadata);

    // Update current_data pointer
    size_t const avail_size = std::distance(current_data, m_end_data);
    current_data =
        (event_size <= avail_size) ?
            current_data + event_size : m_begin_data + event_size - avail_size;

    // Update counters
    ++i;
    ++m_current_event_id;
    m_avail_data -= event_size;

    // Next event size
    event_size = roundUpPowerOf2<data_padding>(
        sizeof(header) + m_payload_size_generator.generate());

  }
  return i;
}

}
