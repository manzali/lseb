#ifndef RU_READOUT_UNIT_H
#define RU_READOUT_UNIT_H

#include <sys/uio.h>

#include <boost/lockfree/spsc_queue.hpp>

#include "common/configuration.h"

namespace lseb {

class ReadoutUnit {
  Configuration m_configuration;
  boost::lockfree::spsc_queue<iovec>& m_free_local_data;
  boost::lockfree::spsc_queue<iovec>& m_ready_local_data;
  int m_id;

 public:
  ReadoutUnit(
    Configuration const& configuration,
    int id,
    boost::lockfree::spsc_queue<iovec>& free_local_data,
    boost::lockfree::spsc_queue<iovec>& ready_local_data);
  void operator()();
};

}

#endif
