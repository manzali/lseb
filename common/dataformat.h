#ifndef COMMON_DATAFORMAT_H
#define COMMON_DATAFORMAT_H

#include <cstdint> // uint64_t

#include "common/utility.h"

namespace lseb {

struct EventMetaData {
  uint64_t id;
  uint64_t length;
  EventMetaData(uint64_t id, uint64_t length)
      :
        id(id),
        length(length) {
  }
};

struct EventHeader {
  uint64_t id;
  uint64_t length;
  uint64_t flags;
  EventHeader(uint64_t id, uint64_t length, uint64_t flags)
      :
        id(id),
        length(length),
        flags(flags) {
  }
};

template<typename T>
class Range {
 public:
  using value_type = T;
  using iterator = T*;
  using const_iterator = T const*;

 private:
  iterator m_begin;
  iterator m_end;

 public:
  Range(iterator begin, iterator end)
      :
        m_begin(begin),
        m_end(end) {
    assert(is_aligned_for<T>(m_begin));
    assert(is_aligned_for<T>(m_end));
  }
  iterator begin() {
    return m_begin;
  }
  iterator end() {
    return m_end;
  }
  const_iterator begin() const {
    return m_begin;
  }
  const_iterator end() const {
    return m_end;
  }
};

template<typename T>
class Buffer {
 public:
  using value_type = T;
  using iterator = T*;
  using const_iterator = T const*;

 private:
  iterator m_begin;
  iterator m_end;
  iterator m_next_read;
  iterator m_next_write;

 public:

  Buffer(void* begin, void* end)
      :
        m_begin(pointer_cast<T>(begin)),
        m_end(pointer_cast<T>(end)),
        m_next_read(pointer_cast<T>(begin)),
        m_next_write(pointer_cast<T>(begin)) {
    assert(m_begin < m_end);
  }
  Buffer(iterator begin, iterator end)
      :
        m_begin(begin),
        m_end(end),
        m_next_read(begin),
        m_next_write(begin) {
    assert(m_begin < m_end);
  }
  iterator begin() {
    return m_begin;
  }
  iterator end() {
    return m_end;
  }
  iterator next_read() {
    return m_next_read;
  }
  iterator next_write() {
    return m_next_write;
  }
  size_t size() {
    return std::distance(m_begin, m_end);
  }
  size_t ready() {
    return (size() + std::distance(m_next_read, m_next_write)) % size();
  }
  size_t available() {
    return size() - ready();
  }
  void release(size_t n) {
    assert(ready() >= n);
    m_next_read = m_begin + (m_next_read - m_begin + n) % size();
  }
  void reserve(size_t n) {
    assert(available() > n);
    m_next_write = m_begin + (m_next_write - m_begin + n) % size();
  }
};

using MetaDataRange = Range<EventMetaData>;
using DataRange = Range<unsigned char>;

using MetaDataBuffer = Buffer<EventMetaData>;
using DataBuffer = Buffer<unsigned char>;

using MultiEvents = std::vector<std::pair<MetaDataRange, DataRange> >;

}

#endif
