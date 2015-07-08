#ifndef RU_SENDER_H
#define RU_SENDER_H

#include <vector>

#include "common/dataformat.h"

#include "transport/transport.h"

namespace lseb {

class Sender {
  std::vector<RuConnectionId> m_connection_ids;
  std::vector<RuConnectionId>::iterator m_next_bu;

 public:
  Sender(std::vector<RuConnectionId> const& connection_ids);
  size_t send(int conn_id, std::vector<DataIov> const& data_iovecs);
};

}

#endif
