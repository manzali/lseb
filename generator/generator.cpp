#include "generator/generator.h"

#include <cmath>
#include <cassert>

#include "commons/pointer_cast.h"
#include "commons/utility.h"

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
      m_end_metadata(end_metadata),
      m_metadata_capacity(std::distance(begin_metadata, end_metadata)),
      m_read_metadata(m_begin_metadata),
      m_write_metadata(m_begin_metadata),
      m_begin_data(begin_data),
      m_end_data(end_data),
      m_data_capacity(std::distance(begin_data, end_data)),
      m_avail_data(m_data_capacity),
      m_current_event_id(0),
      is_empty(true) {
  assert(m_metadata_capacity > 0);
  assert(m_data_capacity > 0);
  assert(data_padding >= sizeof(EventHeader));
  assert(m_data_capacity % data_padding == 0);
}

void Generator::releaseEvents(size_t n_events) {

  size_t const releasable_metadata =
      !is_empty && m_read_metadata == m_write_metadata ?
          m_metadata_capacity :
          circularDistance(m_read_metadata, m_write_metadata,
                           m_metadata_capacity);

  assert(
      n_events <= releasable_metadata && "Wrong number of events to release");

  if (n_events) {
    EventMetaData* last_read_metadata = circularForward(m_read_metadata,
                                                        m_begin_metadata,
                                                        m_metadata_capacity,
                                                        n_events - 1);
    m_avail_data += m_data_capacity
        + (m_data_capacity + last_read_metadata->offset
            + last_read_metadata->length - m_read_metadata->offset)
            % m_data_capacity;
    m_read_metadata = circularForward(last_read_metadata, m_begin_metadata,
                                      m_metadata_capacity, 1);
    if (m_read_metadata == m_write_metadata) {
      is_empty = true;
      m_avail_data = m_data_capacity;
    }

  }
}

size_t Generator::generateEvents(size_t n_events) {

  // Set current data pointer (next to be written)
  size_t data_offset = 0;
  if (!is_empty) {
    EventMetaData* previous_metadata = circularForward(m_write_metadata,
                                                       m_begin_metadata,
                                                       m_metadata_capacity, -1);
    data_offset = previous_metadata->offset + previous_metadata->length;
  }
  char* current_data = m_begin_data + data_offset % m_data_capacity;

  // Start creating events
  size_t i = 0;
  size_t event_size = roundUpPowerOf2<data_padding>(
      sizeof(EventHeader) + m_payload_length_generator.generate());
  size_t const writable_metadata =
      is_empty ?
          m_metadata_capacity :
          circularDistance(m_write_metadata, m_read_metadata,
                           m_metadata_capacity);
  size_t const e =
      (n_events <= writable_metadata) ? n_events : writable_metadata;

  while (i != e && m_avail_data >= event_size) {

    ssize_t offset = std::distance(m_begin_data, current_data);
    assert(offset >= 0);

    // Set EventMetaData
    EventMetaData& metadata = *(new (m_write_metadata) EventMetaData(
        m_current_event_id, event_size, offset));

    // Set EventHeader
    EventHeader& header =
        *(new (pointer_cast<EventHeader>(current_data)) EventHeader(
            m_current_event_id, metadata.length));

    m_write_metadata = circularForward(m_write_metadata, m_begin_metadata,
                                       m_metadata_capacity, 1);

    current_data = circularForward(current_data, m_begin_data, m_data_capacity,
                                   event_size);

    // Update counters
    ++i;
    ++m_current_event_id;
    m_avail_data -= event_size;

    // Next event size
    event_size = roundUpPowerOf2<data_padding>(
        sizeof(header) + m_payload_length_generator.generate());

  }

  if (is_empty && i) {
    is_empty = false;
  }

  assert(n_events >= i && "Generated too much events");

  return i;
}

}
