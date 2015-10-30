#ifndef COMMON_POINTER_CAST_H
#define COMMON_POINTER_CAST_H

#include <type_traits>

#include <cassert>

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

}

#endif
