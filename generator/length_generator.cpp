#include "generator/length_generator.h"

#include <boost/algorithm/clamp.hpp>

#include <cassert>

namespace lseb {

LengthGenerator::LengthGenerator(
  size_t mean,
  size_t stddev,
  size_t max,
  size_t min)
    :
      m_generator(std::random_device { }()),
      m_distribution(mean, stddev),
      m_max(max ? max : mean + 5 * stddev),
      m_min(stddev ? min : mean) {
  assert(m_min <= mean && mean <= m_max);
}

size_t LengthGenerator::generate() {
  return boost::algorithm::clamp(
    static_cast<size_t>(m_distribution(m_generator)),
    m_min,
    m_max);
}

}
