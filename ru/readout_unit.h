#ifndef RU_READOUT_UNIT_H
#define RU_READOUT_UNIT_H

#include <map>

#include <sys/uio.h>

#include <boost/lockfree/spsc_queue.hpp>

#include "ru/accumulator.h"

#include "transport/transport.h"
#include "transport/endpoints.h"

namespace lseb {

class ReadoutUnit {
  Accumulator& m_accumulator;
  boost::lockfree::spsc_queue<iovec>& m_free_local_queue;
  boost::lockfree::spsc_queue<iovec>& m_ready_local_queue;
  std::vector<Endpoint> m_endpoints;
  std::map<int, std::unique_ptr<SendSocket> > m_connection_ids;
  int m_bulk_size;
  int m_credits;
  int m_id;
  int m_pending_local_iov;
  Connector<SendSocket> m_connector;

 public:
  ReadoutUnit(
    Accumulator& accumulator,
    boost::lockfree::spsc_queue<iovec>& free_local_data,
    boost::lockfree::spsc_queue<iovec>& ready_local_data,
    std::vector<Endpoint> const& endpoints,
    int bulk_size,
    int credits,
    int id);
  void connect();
  void run();
};

}

#endif
