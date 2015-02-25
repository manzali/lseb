#ifndef COMMON_DATAFORMAT_H
#define COMMON_DATAFORMAT_H

#include <cstdint> // uint64_t

#include "common/utility.h"

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

class MetaDataRange {
  EventMetaData* m_begin;
  EventMetaData* m_end;

 public:
  using iterator = EventMetaData*;
  MetaDataRange(EventMetaData* begin, EventMetaData* end)
      : m_begin(begin),
        m_end(end) {
  }
  ssize_t size() {
    return std::distance(m_begin, m_end);
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
  EventMetaData* end()const  {
    return m_end;
  }
};

class DataRange {
  unsigned char* m_begin;
  unsigned char* m_end;

 public:
  using iterator = unsigned char*;
  DataRange(unsigned char* begin, unsigned char* end)
      : m_begin(begin),
        m_end(end) {
    assert(is_aligned_for<EventHeader>(begin));
    assert(is_aligned_for<EventHeader>(end));
  }
  ssize_t size() {
    return std::distance(m_begin, m_end);
  }
  unsigned char* begin() {
    return m_begin;
  }
  unsigned char* end() {
    return m_end;
  }
  unsigned char* begin() const {
    return m_begin;
  }
  unsigned char* end()const  {
    return m_end;
  }
};

class MetaDataBuffer {
  EventMetaData* m_begin;
  EventMetaData* m_end;
  EventMetaData* m_next_read;
  EventMetaData* m_next_write;

 public:
  using iterator = EventMetaData*;
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
  MetaDataBuffer(MetaDataRange const& metadata_range)
      : m_begin(metadata_range.begin()),
        m_end(metadata_range.end()),
        m_next_read(metadata_range.begin()),
        m_next_write(metadata_range.begin()) {
    assert(m_begin < m_end);
  }
  EventMetaData* begin() {
    return m_begin;
  }
  EventMetaData* end() {
    return m_end;
  }
  EventMetaData* next_read() {
    return m_next_read;
  }
  EventMetaData* next_write() {
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
  using iterator = unsigned char*;
  DataBuffer(unsigned char* begin, unsigned char* end)
      : m_begin(begin),
        m_end(end),
        m_next_read(begin),
        m_next_write(begin) {
    assert(is_aligned_for<EventHeader>(begin));
    assert(is_aligned_for<EventHeader>(end));
    assert(m_begin < m_end);
  }
  DataBuffer(DataRange const& data_range)
      : m_begin(data_range.begin()),
        m_end(data_range.end()),
        m_next_read(data_range.begin()),
        m_next_write(data_range.begin()) {
    assert(m_begin < m_end);
  }
  unsigned char* begin() {
    return m_begin;
  }
  unsigned char* end() {
    return m_end;
  }
  unsigned char* next_read() {
    return m_next_read;
  }
  unsigned char* next_write() {
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
