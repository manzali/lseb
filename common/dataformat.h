#ifndef COMMON_DATAFORMAT_H
#define COMMON_DATAFORMAT_H

#include <cstdint> // uint64_t

#include "common/utility.h"

namespace lseb {

struct EventMetaData {
  uint64_t id;
  uint64_t length;
  uint64_t offset;
  EventMetaData(uint64_t id, uint64_t length, uint64_t offset)
      :
        id(id),
        length(length),
        offset(offset) {
  }
};

struct EventHeader {
  uint64_t id;
  uint64_t length;
  uint64_t flags;
  EventHeader(uint64_t id, uint64_t length, uint64_t flags)
      :
        id(id),
        length(length),
        flags(flags) {
  }
};

using MetaDataRange = Range<EventMetaData>;
using DataRange = Range<unsigned char>;

using MetaDataBuffer = Buffer<EventMetaData>;
using DataBuffer = Buffer<unsigned char>;

using MultiEvent = std::pair<MetaDataRange, DataRange>;

}

#endif
