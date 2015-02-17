#ifndef COMMONS_UTILITY_H
#define COMMONS_UTILITY_H

#include <iterator>

namespace lseb {

template<typename T>
size_t circularDistance(T begin, T end, size_t capacity) {
  return (capacity + std::distance(begin, end)) % capacity;
}

template<typename T>
T circularForward(T current, T begin, size_t capacity, ssize_t forward) {
  return begin + (current - begin + forward) % capacity;
}

}

#endif
