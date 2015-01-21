#ifndef COMMONS_DATAFORMAT_H
#define COMMONS_DATAFORMAT_H

#include <cstdint> // uint64_t
#include <cstdlib> // size_t

static size_t const payload_size = 1024;

struct MetaData {
  uint64_t id;
  uint64_t length;  // of the event (it will be variable)
  uint64_t offset;  // from the starting pointer of data shared memory
};

struct Data {
  uint64_t length;
  uint64_t flags;
  uint64_t id;
  char payload[payload_size];
};

inline MetaData* metadata_cast(char* pointer) {
  return static_cast<MetaData*>(
    static_cast<void*>(pointer));
}

inline const MetaData* metadata_cast(const char* pointer) {
  return static_cast<const MetaData*>(
    static_cast<const void*>(pointer));
}

inline Data* data_cast(char* pointer) {
  return static_cast<Data*>(
    static_cast<void*>(pointer));
}

inline const Data* data_cast(const char* pointer) {
  return static_cast<const Data*>(
    static_cast<const void*>(pointer));
}

#endif
