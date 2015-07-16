#include "generator/generator.h"

#include <cmath>
#include <cassert>

#include "common/utility.h"
#include "common/log.hpp"

namespace lseb {

static size_t const data_padding = 32;

size_t roundUpPowerOf2(size_t val, size_t n) {
  assert((n & (n - 1)) == 0 && "N must be a power of 2");
  return (val + n - 1) & ~(n - 1);
}

Generator::Generator(
  LengthGenerator const& length_generator,
  MetaDataBuffer const& metadata_buffer,
  DataBuffer const& data_buffer,
  size_t id)
    :
      m_length_generator(length_generator),
      m_metadata_buffer(metadata_buffer),
      m_data_buffer(data_buffer),
      m_id(id) {

  assert(data_padding >= sizeof(EventHeader));
  assert(m_data_buffer.size() % data_padding == 0);

  uint64_t events_counter = 0;
  uint64_t offset = 0;

  bool metadata_avail = true;
  while (metadata_avail) {

    size_t event_size = roundUpPowerOf2(
      sizeof(EventHeader) + m_length_generator.generate(),
      data_padding);

    if (m_data_buffer.available() < event_size + (m_metadata_buffer.available() - 1) * sizeof(EventHeader)) {
      event_size = sizeof(EventHeader);
    }

    if (m_metadata_buffer.available() == 1) {
      event_size = m_data_buffer.available();
      metadata_avail = false;
    }

    // Set EventMetaData
    EventMetaData& metadata =
      *(new (m_metadata_buffer.next_write()) EventMetaData(
        events_counter,
        event_size,
        offset));

    // Set EventHeader
    EventHeader& header = *(new (
      pointer_cast<EventHeader>(m_data_buffer.next_write())) EventHeader(
      events_counter,
      metadata.length,
      m_id));
    /*
     DataBuffer::iterator it = advance_in_range(
     std::begin(m_data_buffer),
     offset,
     m_data_buffer);
     assert(it == m_data_buffer.next_write());
     */
    // The buffer can not be completely filled
    if (m_metadata_buffer.available() != 1) {
      m_metadata_buffer.reserve(1);
      m_data_buffer.reserve(event_size);
    }

    offset = (offset + event_size) % m_data_buffer.size();
    ++events_counter;
  }

  m_metadata_buffer.release(m_metadata_buffer.ready());
  m_data_buffer.release(m_data_buffer.ready());

// Check that all memory is free
  assert(!m_metadata_buffer.ready() && !m_data_buffer.ready());
}

void Generator::releaseEvents(size_t n_events) {

  assert(m_metadata_buffer.ready() >= n_events);

  if (n_events) {
    // Release metadata
    m_metadata_buffer.release(n_events - 1);
    EventMetaData const& metadata = *(pointer_cast<EventMetaData>(
      m_metadata_buffer.next_read()));
    m_metadata_buffer.release(1);

    // Release data
    DataBuffer::iterator data_it = advance_in_range(
      std::begin(m_data_buffer),
      (metadata.offset + metadata.length) % m_data_buffer.size(),
      m_data_buffer);
    DataRange data_range(m_data_buffer.next_read(), data_it);
    size_t n_bytes = distance_in_range(data_range, m_data_buffer);
    m_data_buffer.release(n_bytes);
  }
}

size_t Generator::generateEvents(size_t n_events) {

  // The buffer can not be completely filled
  size_t avail_events =
      (n_events <= m_metadata_buffer.available() - 1) ?
        n_events :
        m_metadata_buffer.available() - 1;

  if (avail_events) {
    // Release metadata
    m_metadata_buffer.reserve(avail_events - 1);
    EventMetaData const& metadata = *(pointer_cast<EventMetaData>(
      m_metadata_buffer.next_write()));
    m_metadata_buffer.reserve(1);

    // Release data
    DataBuffer::iterator data_it = advance_in_range(
      std::begin(m_data_buffer),
      (metadata.offset + metadata.length) % m_data_buffer.size(),
      m_data_buffer);
    DataRange data_range(m_data_buffer.next_write(), data_it);
    size_t n_bytes = distance_in_range(data_range, m_data_buffer);
    m_data_buffer.reserve(n_bytes);
  } else if (n_events) {
    LOG(WARNING) << "Not enough space for events generation!";
  }

  return avail_events;
}

}
