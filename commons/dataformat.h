#ifndef COMMONS_DATAFORMAT_H
#define COMMONS_DATAFORMAT_H

#include <cstdint> // uint64_t

#include "commons/pointer_cast.h"

namespace lseb {

struct EventMetaData {
  uint64_t id;
  uint64_t length;  // of the event (it will be variable)
  uint64_t offset;  // from the starting pointer of data shared memory
  EventMetaData(uint64_t id, uint64_t length, uint64_t offset)
      : id(id),
        length(length),
        offset(offset) {
  }
};

struct EventHeader {
  uint64_t length;
  uint64_t flags;
  uint64_t id;
  EventHeader(uint64_t length, uint64_t id)
      : length(length),
        flags(0),
        id(id) {
  }
};

}

#endif
