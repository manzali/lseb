#ifndef COMMON_UTILITY_H
#define COMMON_UTILITY_H

#include <iostream>
#include <vector>
#include <iterator>
#include <type_traits>
#include <numeric>
#include <random>

#include <cassert>

#include <sys/uio.h>

namespace lseb {

template<typename T>
bool is_aligned_for(void* p) {
  return (reinterpret_cast<uintptr_t>(p) & (std::alignment_of<T>::value - 1)) == 0;
}

template<typename T>
bool is_aligned_for(void const* p) {
  return (reinterpret_cast<uintptr_t>(p) & (std::alignment_of<T>::value - 1)) == 0;
}

template<typename T>
T* pointer_cast(void* p) {
  assert(is_aligned_for<T>(p));
  return static_cast<T*>(p);
}

template<typename T>
T* pointer_cast(void const* p) {
  assert(is_aligned_for<T>(p));
  return static_cast<T*>(p);
}

template<typename R, typename T>
size_t distance_in_range(R sub_range, T range) {
  auto const d = std::distance(std::begin(sub_range), std::end(sub_range));
  return d < 0 ? d + std::distance(std::begin(range), std::end(range)) : d;
}

template<typename R>
typename R::iterator advance_in_range(
  typename R::iterator current,
  ssize_t advance,
  R& range) {
  assert(std::begin(range) < std::end(range));
  assert(std::begin(range) <= current && current < std::end(range));
  auto const size = std::distance(std::begin(range), std::end(range));
  auto const offset =
    (std::distance(std::begin(range), current) + advance) % size;
  return std::begin(range) + offset + (offset < 0 ? size : 0);
}

template<typename R, typename T, typename InitType, typename BinaryOperation>
InitType accumulate_in_range(
  R sub_range,
  T range,
  InitType init,
  BinaryOperation op) {
  if (std::begin(sub_range) < std::end(sub_range)) {
    return std::accumulate(std::begin(sub_range), std::end(sub_range), init, op);
  } else {
    init = std::accumulate(std::begin(sub_range), std::end(range), init, op);
    return std::accumulate(std::begin(range), std::end(sub_range), init, op);
  }
}

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

// http://stackoverflow.com/questions/6942273/get-random-element-from-container-c-stl
template<typename Iter>
Iter select_randomly(Iter start, Iter end) {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, std::distance(start, end) - 1);
  return std::next(start, dis(gen));
}

using DataIov = std::vector<iovec>;

template<typename R, typename T>
DataIov create_iovec(R sub_range, T range) {
  DataIov iov;
  size_t const size = sizeof(typename T::value_type);
  if (std::begin(sub_range) <= std::end(sub_range)) {
    size_t len = distance_in_range(sub_range, range) * size;
    iov.push_back( { std::begin(sub_range), len });
  } else {
    size_t len = std::distance(std::end(sub_range), std::end(range)) * size;
    iov.push_back( { std::end(sub_range), len });
    len = std::distance(std::begin(range), std::begin(sub_range)) * size;
    iov.push_back( { std::begin(range), len });
  }
  return iov;
}

}

#endif
