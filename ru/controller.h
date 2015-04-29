#ifndef RU_CONTROLLER_H
#define RU_CONTROLLER_H

#include <chrono>

#include "common/dataformat.h"
#include "generator/generator.h"

namespace lseb {

class Controller {

  Generator m_generator;
  MetaDataRange m_metadata_range;
  MetaDataRange::iterator m_current_metadata;
  std::chrono::high_resolution_clock::time_point m_start_time;
  size_t m_generator_frequency;
  size_t m_generated_events;

 public:
  Controller(
    Generator const& generator,
    MetaDataRange const& metadata_range,
    size_t generator_frequency);
  MetaDataRange read();
  void release(MetaDataRange metadata_range);

};

}

#endif
