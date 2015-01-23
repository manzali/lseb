#include "generator/Generator.h"

#include <chrono>

#include <cassert>

#include "commons/dataformat.h"

namespace lseb {

template<typename T, typename RandomAccessIterator>
bool enoughSpaceFor(RandomAccessIterator first, RandomAccessIterator last,
                    size_t payload_size = 0) {
  return std::distance(first, last) >= sizeof(T) + payload_size;
}

template<typename T>
T fitToRange(T val, T min, T max) {
  return std::max(min, std::min(val, max));
}

template<size_t N>
size_t roundUpPowerOf2(size_t val) {
  static_assert((N & (N - 1)) == 0, "N must be a power of 2");
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

int Generator::generateEvents(char* begin_metadata, char* end_metadata,
                              char* begin_data, char* end_data) {

  uint64_t const starting_event_id = m_current_event_id;

  size_t payload_size = generatePayloadSize();

  char* current_metadata = begin_metadata;  // I could use begin_metadata instead
  char* current_data = begin_data;  // needed to compute offset

  while (enoughSpaceFor<EventMetaData>(current_metadata, end_metadata)
      && enoughSpaceFor<EventHeader>(current_data, end_data, payload_size)) {

    // Set EventMetaData
    EventMetaData& metadata =
        *(new (EventMetaData_cast(current_metadata)) EventMetaData(
            m_current_event_id, sizeof(EventHeader) + payload_size,
            std::distance(begin_data, current_data)));

    // Set EventHeader
    EventHeader& header = *(new (EventHeader_cast(current_data)) EventHeader(
        metadata.length, m_current_event_id));

    // Update offsets and increment counters
    current_metadata += sizeof(EventMetaData);
    current_data += header.length;
    ++m_current_event_id;

    // Compute new payload size
    payload_size = generatePayloadSize();
  }

  return m_current_event_id - starting_event_id;
}

}
