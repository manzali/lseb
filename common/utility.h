#ifndef COMMON_UTILITY_H
#define COMMON_UTILITY_H

#include <iostream>
#include <vector>
#include <iterator>
#include <type_traits>
#include <numeric>

#include <cassert>

#include <sys/uio.h>

namespace lseb {

template<typename T>
bool is_aligned_for(void* p) {
  return (reinterpret_cast<uintptr_t>(p) & (std::alignment_of<T>::value - 1))
      == 0;
}

template<typename T>
bool is_aligned_for(void const* p) {
  return (reinterpret_cast<uintptr_t>(p) & (std::alignment_of<T>::value - 1))
      == 0;
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
typename R::iterator advance_in_range(typename R::iterator current,
                                      ssize_t advance, R& range) {
  assert(std::begin(range) < std::end(range));
  assert(std::begin(range) <= current && current < std::end(range));
  auto const size = std::distance(std::begin(range), std::end(range));
  auto const offset = (std::distance(std::begin(range), current) + advance)
      % size;
  return std::begin(range) + offset + (offset < 0 ? size : 0);
}

template<typename R, typename T, typename InitType, typename BinaryOperation>
InitType accumulate_in_range(R sub_range, T range, InitType init,
                             BinaryOperation op) {
  if (std::begin(sub_range) < std::end(sub_range)) {
    return std::accumulate(std::begin(sub_range), std::end(sub_range), init, op);
  } else {
    init = std::accumulate(std::begin(sub_range), std::end(range), init, op);
    return std::accumulate(std::begin(range), std::end(sub_range), init, op);
  }
}

template<typename R, typename T>
std::vector<iovec> create_iovec(R sub_range, T range) {
  std::vector<iovec> iov;
  size_t const size = sizeof(typename T::value_type);
  if (std::begin(sub_range) <= std::end(sub_range)) {
    size_t len = distance_in_range(sub_range, range) * size;
    iov.push_back( { std::begin(sub_range), len });
  } else {
    size_t len = std::distance(std::begin(sub_range), std::end(range)) * size;
    iov.push_back( { std::begin(sub_range), len });
    len = std::distance(std::begin(range), std::end(sub_range)) * size;
    iov.push_back( { std::begin(range), len });
  }
  return iov;
}

class BoundedInt {
  int m_counter;
  int m_min;
  int m_size;

 public:
  BoundedInt(int min, int max)
      : m_counter(min),
        m_min(min),
        m_size(max - min + 1) {
    assert(min <= max);
  }
  int get() const {
    return m_counter;
  }
  int min() const {
    return m_min;
  }
  int max() const {
    return m_min + m_size - 1;
  }
  BoundedInt& operator+=(int num) {
    auto const offset = (m_counter - m_min + num) % m_size;
    m_counter = m_min + offset + (offset < 0 ? m_size : 0);
    return *this;
  }
  BoundedInt& operator-=(int num) {
    auto const offset = (m_counter - m_min - num) % m_size;
    m_counter = m_min + offset + (offset < 0 ? m_size : 0);
    return *this;
  }
  BoundedInt& operator++() {
    auto const offset = (m_counter - m_min + 1) % m_size;
    m_counter = m_min + offset + (offset < 0 ? m_size : 0);
    return *this;
  }
  BoundedInt& operator--() {
    auto const offset = (m_counter - m_min - 1) % m_size;
    m_counter = m_min + offset + (offset < 0 ? m_size : 0);
    return *this;
  }
  BoundedInt operator++(int) {
    BoundedInt result(*this);
    ++(*this);
    return result;
  }
  BoundedInt operator--(int) {
    BoundedInt result(*this);
    --(*this);
    return result;
  }
};

}

#endif
