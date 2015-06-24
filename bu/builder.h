#ifndef BU_BUILDER_H
#define BU_BUILDER_H

#include <vector>

#include "common/dataformat.h"
#include "common/handler_executor.hpp"

namespace lseb {

class Builder {
  HandlerExecutor m_executor;

 public:
  Builder();
  void build(std::vector<iovec> total_iov);
};

}

#endif
