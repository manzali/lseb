#ifndef COMMONS_DATAFORMAT_H
#define COMMONS_DATAFORMAT_H

#include <iterator>

#include <cstdint> // uint64_t

#include "commons/pointer_cast.h"

namespace lseb {

struct EventMetaData {
  uint64_t id;
  uint64_t length;  // of the event (it will be variable)
  uint64_t offset;  // from the starting pointer of data shared memory
  EventMetaData(uint64_t id, uint64_t length, uint64_t offset)
      : id(id),
        length(length),
        offset(offset) {
  }
};

struct EventHeader {
  uint64_t id;
  uint64_t length;
  uint64_t flags;
  EventHeader(uint64_t id, uint64_t length)
      : id(id),
        length(length),
        flags(0) {
  }
};

class EventMetaDataRange {
  EventMetaData* const m_begin;
  EventMetaData* const m_end;

 public:
  EventMetaDataRange(EventMetaData* begin, EventMetaData* end)
      : m_begin(begin),
        m_end(end) {
  }
  EventMetaData* begin() {
    return m_begin;
  }
  EventMetaData* end() {
    return m_end;
  }
  EventMetaData* begin() const {
    return m_begin;
  }
  EventMetaData* end() const {
    return m_end;
  }
  size_t distance(size_t capacity) {
    return (capacity + std::distance(m_begin, m_end)) % capacity;
  }
};

}

#endif
