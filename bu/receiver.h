#ifndef BU_RECEIVER_H
#define BU_RECEIVER_H

#include <vector>

#include "common/dataformat.h"
#include "common/handler_executor.hpp"

#include "transport/transport.h"

namespace lseb {

class Receiver {
  std::vector<BuConnectionId> m_connection_ids;
  HandlerExecutor m_executor;

 public:
  Receiver(std::vector<BuConnectionId> const& connection_ids);
  size_t receive(int ms_timeout);
  size_t receiveAndForget();
};

}

#endif
