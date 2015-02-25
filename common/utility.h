#ifndef COMMON_UTILITY_H
#define COMMON_UTILITY_H

#include <iterator>
#include <type_traits>

#include <cassert>

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
  auto d = std::distance(std::begin(sub_range), std::end(sub_range));
  return d >= 0 ? d : d + std::distance(std::begin(range), std::end(range));
}

template<typename R>
typename R::iterator advance_in_range(typename R::iterator current,
                                      ssize_t advance, R range) {
  assert(std::begin(range) < std::end(range));
  return std::begin(range)
      + (current - std::begin(range) + advance)
          % std::distance(std::begin(range), std::end(range));
}

}

#endif
