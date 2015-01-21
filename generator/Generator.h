#ifndef GENERATOR_GENERATOR_H
#define GENERATOR_GENERATOR_H

#include <cstdint> // uint64_t
#include <cstdlib> // size_t

class Generator {
 private:
  uint64_t current_event_id_;
  char* const metadata_ptr_;
  size_t const metadata_size_;
  char* const data_ptr_;
  size_t const data_size_;

 public:
  Generator(char* metadata_ptr, size_t metadata_size, char* data_ptr,
            size_t data_size);
  int generateEvents();  // returns number of generated events

};

#endif
