#include "generator/Generator.h"

#include <chrono>

#include <cstring> // memset
#include <cassert>

#include "commons/dataformat.h"

namespace lseb {

bool checkMetaDataSize(size_t size, size_t offset) {
  return offset + sizeof(EventMetaData) <= size;
}

bool checkDataSize(size_t size, size_t offset, size_t payload) {
  return offset + sizeof(EventHeader) + payload <= size;
}

Generator::Generator(size_t mean, size_t stddev, size_t max, size_t min)
    : m_current_event_id(0),
      m_generator(std::chrono::system_clock::now().time_since_epoch().count()),
      m_distribution(mean, stddev),
      m_min(min),
      m_max(max ? max : mean + 5 * stddev) {
  assert(m_min <= mean && mean <= m_max);
}

size_t Generator::generatePayloadSize() {
  size_t const val = m_distribution(m_generator);
  /*
   * If val > max then return max and
   * if val < min then return min,
   * this is to avoid values out of range (i.e. negative)
   */
  return std::max(m_min, std::min(val, m_max));
}

int Generator::generateEvents(char* metadata_ptr, size_t metadata_size,
                              char* data_ptr, size_t data_size) {

  size_t m_offset = 0;
  size_t d_offset = 0;

  uint64_t const starting_event_id = m_current_event_id;

  size_t payload_size = generatePayloadSize();

  while (checkMetaDataSize(metadata_size, m_offset)
      && (checkDataSize(data_size, d_offset, payload_size))) {

    // Set EventMetaData
    EventMetaData& metadata = *eventmetadata_cast(metadata_ptr + m_offset);
    metadata.id = m_current_event_id;
    metadata.length = sizeof(EventHeader) + payload_size;
    metadata.offset = d_offset;

    // Set EventHeader
    EventHeader& header = *eventheader_cast(data_ptr + d_offset);
    header.length = metadata.length;
    header.flags = 0;
    header.id = m_current_event_id;

    // Update offsets and increment counters
    m_offset += sizeof(metadata);
    d_offset += header.length;
    ++m_current_event_id;

    // Compute new payload size
    payload_size = generatePayloadSize();
  }

  return m_current_event_id - starting_event_id;
}

}
