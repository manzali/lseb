#include "generator/Generator.h"

#include <chrono>

#include <cassert>

#include "commons/dataformat.h"

namespace lseb {

bool checkMetaDataSize(size_t size, size_t offset) {
  return offset + sizeof(EventMetaData) <= size;
}

bool checkDataSize(size_t size, size_t offset, size_t payload) {
  return offset + sizeof(EventHeader) + payload <= size;
}

size_t fitToRange(size_t val, size_t min, size_t max) {
  return std::max(min, std::min(val, max));
}

template<int N>
size_t roundUpPowerOf2(size_t val) {
  assert(N > 0 && (N & (N - 1)) == 0 && "N must be a power of 2");
  return (val + N - 1) & ~(N - 1);
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
  return roundUpPowerOf2<8>(fitToRange(val, m_min, m_max));
}

int Generator::generateEvents(char* metadata_ptr, size_t metadata_size,
                              char* data_ptr, size_t data_size) {

  size_t m_offset = 0;
  size_t d_offset = 0;

  uint64_t const starting_event_id = m_current_event_id;

  size_t payload_size = generatePayloadSize();

  while (checkMetaDataSize(metadata_size, m_offset)
      && checkDataSize(data_size, d_offset, payload_size)) {

    // Set EventMetaData
    EventMetaData& metadata = *EventMetaData_cast(metadata_ptr + m_offset);
    metadata.id = m_current_event_id;
    metadata.length = sizeof(EventHeader) + payload_size;
    metadata.offset = d_offset;

    // Set EventHeader
    EventHeader& header = *EventHeader_cast(data_ptr + d_offset);
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
