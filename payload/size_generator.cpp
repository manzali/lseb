#include "payload/size_generator.h"

#include <cassert>

#include <chrono>

namespace lseb {

template<typename T>
T fitToRange(T val, T min, T max) {
  return std::max(min, std::min(val, max));
}

template<size_t N>
size_t roundUpPowerOf2(size_t val) {
  static_assert((N & (N - 1)) == 0, "N must be a power of 2");
  return (val + N - 1) & ~(N - 1);
}

// Constructor for variable size
SizeGenerator::SizeGenerator(size_t mean, size_t stddev,
                             size_t max, size_t min)
    : m_generator(std::chrono::system_clock::now().time_since_epoch().count()),
      m_distribution(mean, stddev),
      m_max(max ? max : mean + 5 * stddev),
      m_min(stddev ? min : mean) {
  assert(m_min <= mean && mean <= m_max);
}

size_t SizeGenerator::generate() {
  size_t const val = m_distribution(m_generator);
  // round up multiple of 8
  return roundUpPowerOf2<8>(fitToRange(val, m_min, m_max));
}

}
