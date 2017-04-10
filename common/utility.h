#ifndef COMMON_UTILITY_H
#define COMMON_UTILITY_H

#include <iostream>
#include <vector>
#include <iterator>
#include <type_traits>
#include <numeric>
#include <random>
#include <algorithm>

#include <cassert>

#include <sys/uio.h>

#include "common/pointer_cast.h"
#include "common/exception.h"

#include "log/log.hpp"

namespace lseb {

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
  Buffer(const_iterator begin, const_iterator end)
      :
        m_begin(const_cast<iterator>(begin)),
        m_end(const_cast<iterator>(end)),
        m_next_read(const_cast<iterator>(begin)),
        m_next_write(const_cast<iterator>(begin)) {
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

// http://stackoverflow.com/questions/6942273/get-random-element-from-container-c-stl
template<typename Iter>
Iter select_randomly(Iter start, Iter end) {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, std::distance(start, end) - 1);
  return std::next(start, dis(gen));
}

inline size_t iovec_length(std::vector<iovec> const& iov) {
  return std::accumulate(
    std::begin(iov),
    std::end(iov),
    0,
    [](size_t partial, iovec const& v) {
      return partial + v.iov_len;});
}

inline std::vector<int> create_sequence(int id, int total) {
  int first_id = (id != total - 1) ? id + 1 : 0;
  std::vector<int> sequence(total);
  std::iota(std::begin(sequence), std::end(sequence), 0);
  std::rotate(
    std::begin(sequence),
    std::begin(sequence) + first_id,
    std::end(sequence));
  return sequence;
}

inline async_log::severity_level severity_from_string(std::string const& str) {
  async_log::severity_level sev = async_log::severity_level::trace;
  if (str == "TRACE"){
    // default value
  }
  else if (str == "DEBUG"){
    sev = async_log::severity_level::debug;
  }
  else if (str == "INFO"){
    sev = async_log::severity_level::info;
  }
  else if (str == "WARNING"){
    sev = async_log::severity_level::warning;
  }
  else if (str == "ERROR"){
    sev = async_log::severity_level::error;
  }
  else if (str == "FATAL"){
    sev = async_log::severity_level::fatal;
  }
  else {
    throw lseb::exception::configuration::generic_error(
        "Error parsing LOG_LEVEL: Unknown " + str + " level!");
  }
  return sev;
}

}

#endif
