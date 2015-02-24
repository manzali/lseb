#include "generator/generator.h"

#include <cmath>
#include <cassert>

#include "common/utility.h"

namespace lseb {

static size_t const data_padding = 32;

template<size_t N>
size_t roundUpPowerOf2(size_t val) {
  static_assert((N & (N - 1)) == 0, "N must be a power of 2");
  return (val + N - 1) & ~(N - 1);
}

Generator::Generator(LengthGenerator const& length_generator,
                     MetaDataBuffer const& metadata_buffer,
                     DataBuffer const& data_buffer)
    : m_length_generator(length_generator),
      m_metadata_buffer(metadata_buffer),
      m_data_buffer(data_buffer),
      m_current_event_id(0) {
  assert(data_padding >= sizeof(EventHeader));
  assert(m_data_buffer.size() % data_padding == 0);
}

void Generator::releaseEvents(size_t n_events) {

  assert(m_metadata_buffer.ready() >= n_events);

  if (n_events) {
    size_t start_data_offset = m_metadata_buffer.next_read()->offset;
    m_metadata_buffer.release(n_events - 1);
    size_t end_data_offset = m_metadata_buffer.next_read()->offset
        + m_metadata_buffer.next_read()->length;
    m_metadata_buffer.release(1);
    size_t data_load = (m_data_buffer.size() + end_data_offset
        - start_data_offset) % m_data_buffer.size();
    m_data_buffer.release(data_load);
  }
}

size_t Generator::generateEvents(size_t n_events) {

  size_t events_count = 0;
  size_t event_size = roundUpPowerOf2<data_padding>(
      sizeof(EventHeader) + m_length_generator.generate());

  size_t const max_events =
      (n_events <= m_metadata_buffer.available()) ?
          n_events : m_metadata_buffer.available();

  while (events_count != max_events && m_data_buffer.available() > event_size) {

    size_t offset = std::distance(m_data_buffer.begin(),
                                  m_data_buffer.next_write());

    // Set EventMetaData
    EventMetaData& metadata =
        *(new (m_metadata_buffer.next_write()) EventMetaData(m_current_event_id,
                                                             event_size, offset));

    // Set EventHeader
    EventHeader& header = *(new (
        pointer_cast<EventHeader>(m_data_buffer.next_write())) EventHeader(
        m_current_event_id, metadata.length));

    m_metadata_buffer.reserve(1);
    m_data_buffer.reserve(event_size);

    // Update counters
    ++events_count;
    ++m_current_event_id;

    // Next event size
    event_size = roundUpPowerOf2<data_padding>(
        sizeof(header) + m_length_generator.generate());

  }

  assert(n_events >= events_count && "Generated too much events");

  return events_count;
}

}
