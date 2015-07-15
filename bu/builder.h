#ifndef BU_BUILDER_H
#define BU_BUILDER_H

#include <vector>

#include "common/dataformat.h"

namespace lseb {

class Builder {
  std::vector<std::vector<iovec> > m_multievents;

 public:
  Builder(int connections);
  void add(int conn, std::vector<iovec> const& conn_iov);
};

}

#endif
