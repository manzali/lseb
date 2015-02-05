#ifndef GENERATOR_GENERATOR_H
#define GENERATOR_GENERATOR_H

#include <random>

#include <cstdint> // uint64_t
#include <cstdlib> // size_t

#include "payload/size_generator.h"
#include "commons/dataformat.h"

namespace lseb {

class Generator {

 private:
  char* const m_begin_data;
  uint64_t m_current_event_id;
  SizeGenerator m_payload_size_generator;

 public:
  Generator(char* begin_data, SizeGenerator const& payload_size_generator);
  uint64_t generateEvents(EventMetaData* current_metadata, EventMetaData* end_metadata,
                         char* current_data, char* end_data);  // returns number of generated events
};

}

#endif
