#ifndef BU_BUILDER_H
#define BU_BUILDER_H

#include <vector>

#include "common/dataformat.h"

namespace lseb {

class Builder {

 public:
  Builder();
  void build(int conn, std::vector<iovec> const& conn_iov);
};

}

#endif
