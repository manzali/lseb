#ifndef RU_READOUT_UNIT_H
#define RU_READOUT_UNIT_H

#include <sys/uio.h>

#include <boost/lockfree/spsc_queue.hpp>

#include "common/configuration.h"
#include "transport/transport.h"

namespace lseb {

class ReadoutUnit {
  Configuration m_configuration;
  boost::lockfree::spsc_queue<iovec>& m_free_local_queue;
  boost::lockfree::spsc_queue<iovec>& m_ready_local_queue;
  std::map<int, SendSocket> m_connection_ids;
  int m_id;
  std::vector<iovec> m_local_data;

 private:
  size_t remote_write(int id, iovec const& iov);
  size_t local_write(iovec const& iov);

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
