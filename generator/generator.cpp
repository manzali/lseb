#include "generator/generator.h"

#include <cmath>
#include <cassert>

#include "common/utility.h"
#include "log/log.hpp"

namespace lseb {

static size_t const data_padding = 32;

Generator::Generator(
  LengthGenerator const& length_generator,
  MetaDataRange const& metadata_range,
  DataRange const& data_range,
  size_t id)
    :
      m_length_generator(length_generator),
      m_metadata_buffer(std::begin(metadata_range), std::end(metadata_range)),
      m_id(id) {

  DataBuffer data_buffer(std::begin(data_range), std::end(data_range));

  assert(data_padding >= sizeof(EventHeader));
  assert(data_buffer.size() % data_padding == 0);

  uint64_t events_counter = 0;
  uint64_t offset = 0;

  bool metadata_avail = true;
  while (metadata_avail) {

    size_t event_size = sizeof(EventHeader) + m_length_generator.generate();
    // round down
    event_size -= (event_size % data_padding);

    if (data_buffer.available() < event_size + (m_metadata_buffer.available() - 1) * sizeof(EventHeader)) {
      event_size = sizeof(EventHeader);
    }

    if (m_metadata_buffer.available() == 1) {
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
      pointer_cast<EventHeader>(data_buffer.next_write())) EventHeader(
      events_counter,
      metadata.length,
      m_id));

    // The buffer can not be completely filled
    if (m_metadata_buffer.available() != 1) {
      m_metadata_buffer.reserve(1);
      data_buffer.reserve(event_size);
    }

    offset = (offset + event_size) % data_buffer.size();
    ++events_counter;
  }

  m_metadata_buffer.release(m_metadata_buffer.ready());
  data_buffer.release(data_buffer.ready());

  LOG_INFO << "Generator - Capacity of " << events_counter << " events";

// Check that all memory is free
  assert(!m_metadata_buffer.ready() && !data_buffer.ready());
}

void Generator::releaseEvents(size_t n_events) {

  assert(m_metadata_buffer.ready() >= n_events);

  if (n_events) {
    // Release metadata
    m_metadata_buffer.release(n_events - 1);
    EventMetaData const& metadata = *(pointer_cast<EventMetaData>(
      m_metadata_buffer.next_read()));
    m_metadata_buffer.release(1);
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
  }/* else if (n_events) {
    LOG(WARNING) << "Generator - Not enough space for events generation!";
  }*/

  return avail_events;
}

}
