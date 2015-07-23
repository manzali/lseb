#ifndef BU_BUILDER_UNIT_H
#define BU_BUILDER_UNIT_H

#include <sys/uio.h>

#include "common/configuration.h"
#include "common/shared_queue.h"

namespace lseb {

class BuilderUnit {
  Configuration m_configuration;
  SharedQueue<std::vector<iovec> >& m_free_local_data;
  SharedQueue<std::vector<iovec> >& m_ready_local_data;
  int m_id;

 public:
  BuilderUnit(
    Configuration const& configuration,
    int id,
    SharedQueue<std::vector<iovec> >& free_local_data,
    SharedQueue<std::vector<iovec> >& ready_local_data);
  void operator()();
};

}

#endif
