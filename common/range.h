#ifndef COMMON_RANGE_H
#define COMMON_RANGE_H

#include <iterator>
#include <type_traits>
#include <numeric>
#include <random>
#include <algorithm>

#include <cassert>

#include "common/pointer_cast.h"

namespace lseb {

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

template<typename R, typename T, typename BinaryOperation>
typename R::iterator find_in_range(R sub_range, T range, BinaryOperation op) {
  if (std::begin(sub_range) < std::end(sub_range)) {
    return std::find_if(std::begin(sub_range), std::end(sub_range), op);
  } else {
    typename R::iterator it = std::find_if(
      std::begin(sub_range),
      std::end(range),
      op);
    return
        (it == std::end(range)) ?
          std::find_if(std::begin(range), std::end(sub_range), op) :
          it;
  }
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

}

#endif
