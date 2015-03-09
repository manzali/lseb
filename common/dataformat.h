#ifndef COMMON_DATAFORMAT_H
#define COMMON_DATAFORMAT_H

#include <cstdint> // uint64_t

#include "common/utility.h"

namespace lseb {

struct EventMetaData {
  uint64_t id;
  uint64_t length;  // of the event (it will be variable)
  EventMetaData(uint64_t id, uint64_t length)
      : id(id),
        length(length) {
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

class MetaDataRange {
  EventMetaData* m_begin;
  EventMetaData* m_end;

 public:
  using value_type = EventMetaData;
  using iterator = EventMetaData*;
  using const_iterator = EventMetaData const*;
  MetaDataRange(EventMetaData* begin, EventMetaData* end)
      : m_begin(begin),
        m_end(end) {
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

class DataRange {
  unsigned char* m_begin;
  unsigned char* m_end;

 public:
  using value_type = unsigned char;
  using iterator = unsigned char*;
  using const_iterator = unsigned char const*;
  DataRange(unsigned char* begin, unsigned char* end)
      : m_begin(begin),
        m_end(end) {
    assert(is_aligned_for<EventHeader>(begin));
    assert(is_aligned_for<EventHeader>(end));
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

class MetaDataBuffer {
  EventMetaData* m_begin;
  EventMetaData* m_end;
  EventMetaData* m_next_read;
  EventMetaData* m_next_write;

 public:
  using value_type = EventMetaData;
  using iterator = EventMetaData*;
  using const_iterator = EventMetaData const*;
  MetaDataBuffer(unsigned char* begin, unsigned char* end)
      : m_begin(pointer_cast<EventMetaData>(begin)),
        m_end(pointer_cast<EventMetaData>(end)),
        m_next_read(pointer_cast<EventMetaData>(begin)),
        m_next_write(pointer_cast<EventMetaData>(begin)) {
    assert(m_begin < m_end);
  }
  MetaDataBuffer(EventMetaData* begin, EventMetaData* end)
      : m_begin(begin),
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
  void release(size_t n_events) {
    assert(ready() >= n_events);
    m_next_read = m_begin + (m_next_read - m_begin + n_events) % size();
  }
  void reserve(size_t n_events) {
    assert(available() > n_events);
    m_next_write = m_begin + (m_next_write - m_begin + n_events) % size();
  }
};

class DataBuffer {
  unsigned char* m_begin;
  unsigned char* m_end;
  unsigned char* m_next_read;
  unsigned char* m_next_write;

 public:
  using value_type = unsigned char;
  using iterator = unsigned char*;
  using const_iterator = unsigned char const*;
  DataBuffer(unsigned char* begin, unsigned char* end)
      : m_begin(begin),
        m_end(end),
        m_next_read(begin),
        m_next_write(begin) {
    assert(is_aligned_for<EventHeader>(begin));
    assert(is_aligned_for<EventHeader>(end));
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
  void release(size_t n_bytes) {
    assert(ready() >= n_bytes);
    m_next_read = m_begin + (m_next_read - m_begin + n_bytes) % size();
    assert(is_aligned_for<EventHeader>(m_next_read));
  }
  void reserve(size_t n_bytes) {
    assert(available() > n_bytes);
    m_next_write = m_begin + (m_next_write - m_begin + n_bytes) % size();
    assert(is_aligned_for<EventHeader>(m_next_write));
  }
};

}

#endif
