#ifndef BU_BUILDER_H
#define BU_BUILDER_H

#include <vector>

#include "common/dataformat.h"

namespace lseb {

class Builder {

 public:
  Builder();
  void build(std::vector<iovec> total_iov);
};

}

#endif
