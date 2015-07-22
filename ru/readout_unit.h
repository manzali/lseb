#ifndef RU_READOUT_UNIT_H
#define RU_READOUT_UNIT_H

#include <sys/uio.h>

#include "common/configuration.h"
#include "common/shared_queue.h"

namespace lseb {

class ReadoutUnit {
  Configuration m_configuration;
  SharedQueue<iovec>& m_free_local_data;
  SharedQueue<iovec>& m_ready_local_data;
  int m_id;

 public:
  ReadoutUnit(
    Configuration const& configuration,
    int id,
    SharedQueue<iovec>& free_local_data,
    SharedQueue<iovec>& ready_local_data);
  void operator()();
};

}

#endif
