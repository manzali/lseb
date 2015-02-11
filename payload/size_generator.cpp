#include "payload/size_generator.h"

#include <cassert>

namespace lseb {

template<typename T>
T fitToRange(T val, T min, T max) {
  return std::max(min, std::min(val, max));
}

SizeGenerator::SizeGenerator(size_t mean, size_t stddev, size_t max, size_t min)
    : m_generator(std::random_device { }()),
      m_distribution(mean, stddev),
      m_max(max ? max : mean + 5 * stddev),
      m_min(stddev ? min : mean) {
  assert(m_min <= mean && mean <= m_max);
}

size_t SizeGenerator::generate() {
  return fitToRange(
      static_cast<size_t>(m_distribution(m_generator)), m_min, m_max);
}

}
