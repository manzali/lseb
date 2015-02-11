#ifndef GENERATOR_GENERATOR_H
#define GENERATOR_GENERATOR_H

#include <utility>

#include <cstdlib> // size_t

#include <boost/circular_buffer.hpp>

#include "payload/length_generator.h"
#include "commons/dataformat.h"

namespace lseb {

class Generator {

 private:
  LengthGenerator m_payload_length_generator;
  EventMetaData* const m_begin_metadata;
  boost::circular_buffer<EventMetaData*> m_ring_metadata;
  char* const m_begin_data;
  char* const m_end_data;
  size_t m_avail_data;
  size_t m_current_event_id;

 public:
  Generator(LengthGenerator const& payload_length_generator,
            EventMetaData* begin_metadata, EventMetaData* end_metadata,
            char* begin_data, char* end_data);
  size_t releaseEvents(size_t n_events);
  size_t generateEvents(size_t n_events);
};

}

#endif
