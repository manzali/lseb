#ifndef GENERATOR_GENERATOR_H
#define GENERATOR_GENERATOR_H

#include <cstdlib> // size_t

#include "../common/dataformat.h"
#include "generator/length_generator.h"

namespace lseb {

class Generator {
  LengthGenerator m_length_generator;
  MetaDataBuffer m_metadata_buffer;
  DataBuffer m_data_buffer;
  size_t m_current_event_id;

 public:
  Generator(LengthGenerator const& length_generator,
            MetaDataBuffer const& metadata_buffer,
            DataBuffer const& data_buffer);
  void releaseEvents(size_t n_events);
  size_t generateEvents(size_t n_events);
};

}

#endif
