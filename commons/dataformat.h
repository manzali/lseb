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

inline EventMetaData* eventmetadata_cast(char* pointer) {
  return static_cast<EventMetaData*>(static_cast<void*>(pointer));
}

inline EventMetaData const* eventmetadata_cast(char const* pointer) {
  return static_cast<const EventMetaData*>(static_cast<const void*>(pointer));
}

inline EventHeader* eventheader_cast(char* pointer) {
  return static_cast<EventHeader*>(static_cast<void*>(pointer));
}

inline EventHeader const* eventheader_cast(char const* pointer) {
  return static_cast<const EventHeader*>(static_cast<const void*>(pointer));
}

}

#endif
