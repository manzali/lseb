#ifndef GENERATOR_LENGTH_GENERATOR_H
#define GENERATOR_LENGTH_GENERATOR_H

#include <random>

#include <cstdlib> // size_t

namespace lseb {

class LengthGenerator {

 private:
  std::default_random_engine m_generator;
  std::normal_distribution<> m_distribution;
  size_t m_max;
  size_t m_min;

 public:
  LengthGenerator(size_t mean, size_t stddev = 0, size_t max = 0,
                  size_t min = 0);
  size_t generate();
};

}

#endif
