#ifndef GENERATOR_GENERATOR_H
#define GENERATOR_GENERATOR_H

#include <utility>

#include <random>

#include <cstdint> // uint64_t
#include <cstdlib> // size_t

namespace lseb {

class Generator {
 private:
  uint64_t m_current_event_id;
  std::default_random_engine m_generator;
  std::normal_distribution<> m_distribution;
  size_t m_min;
  size_t m_max;

 private:
  size_t generatePayloadSize();

 public:
  Generator(size_t mean, size_t stddev, size_t max = 0, size_t min = 0);
  std::pair<char*, char*> generateEvents(char* begin_metadata,
                                         char* end_metadata, char* begin_data,
                                         char* end_data);  // returns number of generated events
};

}

#endif
