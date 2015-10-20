#ifndef BU_BUILDER_UNIT_H
#define BU_BUILDER_UNIT_H

#include <sys/uio.h>

#include <boost/lockfree/spsc_queue.hpp>

#include "common/configuration.h"
#include "transport/transport.h"

namespace lseb {

class BuilderUnit {
  Configuration m_configuration;
  boost::lockfree::spsc_queue<iovec>& m_free_local_queue;
  boost::lockfree::spsc_queue<iovec>& m_ready_local_queue;
  std::vector<RecvSocket> m_connection_ids;
  int m_id;

 public:
  BuilderUnit(
    Configuration const& configuration,
    int id,
    boost::lockfree::spsc_queue<iovec>& free_local_data,
    boost::lockfree::spsc_queue<iovec>& ready_local_data);
  void operator()();
};

}

#endif
