#ifndef RU_SENDER_H
#define RU_SENDER_H

#include "common/dataformat.h"
#include "common/timer.h"
#include "transport/transport.h"

namespace lseb {

class Sender {
  std::vector<RuConnectionId> m_connection_ids;
  std::vector<RuConnectionId>::iterator m_next_bu;
  size_t m_recv_buffer_size;
  Timer m_send_timer;

 public:
  Sender(
    std::vector<RuConnectionId> const& connection_ids,
    size_t max_sending_size);
  size_t send(std::vector<DataIov> data_iovecs);
};

}

#endif
