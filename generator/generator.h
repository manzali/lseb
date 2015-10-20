#ifndef GENERATOR_GENERATOR_H
#define GENERATOR_GENERATOR_H

#include <cstdlib> // size_t

#include "common/dataformat.h"
#include "generator/length_generator.h"

namespace lseb {

class Generator {
  LengthGenerator m_length_generator;
  MetaDataBuffer m_metadata_buffer;
  size_t m_id;

 public:
  Generator(
    LengthGenerator const& length_generator,
    MetaDataRange const& metadata_range,
    DataRange const& data_range,
    size_t id);
  void releaseEvents(size_t n_events);
  size_t generateEvents(size_t n_events);
};

}

#endif
