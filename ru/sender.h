#ifndef RU_SENDER_H
#define RU_SENDER_H

#include <vector>

#include "common/dataformat.h"
#include "common/handler_executor.hpp"

#include "transport/transport.h"

namespace lseb {

class Sender {
  std::vector<RuConnectionId> m_connection_ids;
  std::vector<RuConnectionId>::iterator m_next_bu;
  HandlerExecutor m_executor;

 public:
  Sender(std::vector<RuConnectionId> const& connection_ids);
  size_t send(std::vector<DataIov> data_iovecs);
};

}

#endif
