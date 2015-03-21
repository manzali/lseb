#include "generator/generator.h"

#include <cmath>
#include <cassert>

#include "common/utility.h"
#include "common/log.h"

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

  size_t metadata_avail = m_metadata_buffer.available();
  while (metadata_avail) {

    size_t event_size = roundUpPowerOf2(
      sizeof(EventHeader) + m_length_generator.generate(),
      data_padding);

    if (m_data_buffer.available() < event_size + (m_metadata_buffer.available() - 1) * sizeof(EventHeader)) {
      event_size = sizeof(EventHeader);
    }

    if (m_metadata_buffer.available() == 1) {
      event_size = m_data_buffer.available();
    }

    // Set EventMetaData
    EventMetaData& metadata =
      *(new (m_metadata_buffer.next_write()) EventMetaData(
        events_counter,
        event_size));

    // Set EventHeader
    EventHeader& header = *(new (
      pointer_cast<EventHeader>(m_data_buffer.next_write())) EventHeader(
      events_counter,
      metadata.length,
      m_id));

    // The buffer can not be completely filled
    if (m_metadata_buffer.available() != 1) {
      m_metadata_buffer.reserve(1);
      m_data_buffer.reserve(event_size);
    }

    --metadata_avail;
    ++events_counter;
  }

  m_metadata_buffer.release(m_metadata_buffer.ready());
  m_data_buffer.release(m_data_buffer.ready());

  // Check that all memory is free
  assert(!m_metadata_buffer.ready() && !m_data_buffer.ready());
}

void Generator::releaseEvents(size_t n_events) {

  assert(m_metadata_buffer.ready() >= n_events);

  for (size_t i = 0; i != n_events; ++i) {

    // Set EventMetaData
    EventMetaData const& metadata = *(pointer_cast<EventMetaData>(
      m_metadata_buffer.next_read()));

    m_metadata_buffer.release(1);
    m_data_buffer.release(metadata.length);
  }
}

size_t Generator::generateEvents(size_t n_events) {

  // The buffer can not be completely filled
  size_t avail_events =
      (n_events <= m_metadata_buffer.available() - 1) ?
        n_events :
        m_metadata_buffer.available() - 1;

  for (size_t i = 0; i != avail_events; ++i) {

    // Set EventMetaData
    EventMetaData const& metadata = *(pointer_cast<EventMetaData>(
      m_metadata_buffer.next_write()));

    m_metadata_buffer.reserve(1);
    m_data_buffer.reserve(metadata.length);
  }

  if (n_events && !avail_events) {
    LOG(WARNING) << "Not enough space for events generation!";
  }

  return avail_events;
}

}
