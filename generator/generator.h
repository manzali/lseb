#ifndef GENERATOR_GENERATOR_H
#define GENERATOR_GENERATOR_H

#include <cstdlib> // size_t

#include "payload/length_generator.h"
#include "commons/dataformat.h"

namespace lseb {

class Generator {

  LengthGenerator m_payload_length_generator;
  EventMetaData* const m_begin_metadata;
  EventMetaData* const m_end_metadata;
  std::ptrdiff_t const m_metadata_capacity;
  EventMetaData* m_read_metadata;
  EventMetaData* m_write_metadata;
  char* const m_begin_data;
  char* const m_end_data;
  std::ptrdiff_t const m_data_capacity;
  size_t m_avail_data;
  size_t m_current_event_id;
  bool is_empty;

 public:
  Generator(LengthGenerator const& payload_length_generator,
            EventMetaData* begin_metadata, EventMetaData* end_metadata,
            char* begin_data, char* end_data);
  void releaseEvents(size_t n_events);
  size_t generateEvents(size_t n_events);
};

}

#endif
