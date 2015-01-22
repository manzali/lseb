#ifndef COMMONS_DATAFORMAT_H
#define COMMONS_DATAFORMAT_H

#include <cstdint> // uint64_t
#include <cstdlib> // size_t

namespace lseb {

struct EventMetaData {
  uint64_t id;
  uint64_t length;  // of the event (it will be variable)
  uint64_t offset;  // from the starting pointer of data shared memory
};

struct EventHeader {
  uint64_t length;
  uint64_t flags;
  uint64_t id;
};

inline EventMetaData* EventMetaData_cast(void* pointer) {
  return static_cast<EventMetaData*>(pointer);
}

inline EventMetaData const* EventMetaData_cast(void const* pointer) {
  return static_cast<const EventMetaData*>(pointer);
}

inline EventHeader* EventHeader_cast(void* pointer) {
  return static_cast<EventHeader*>(pointer);
}

inline EventHeader const* EventHeader_cast(void const* pointer) {
  return static_cast<const EventHeader*>(pointer);
}

}

#endif
