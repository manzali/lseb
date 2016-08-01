#ifndef TRANSPORT_VERBS_CONNECTOR_H
#define TRANSPORT_VERBS_CONNECTOR_H

#include <memory>

#include <rdma/rdma_cma.h>

#include "transport/verbs/socket.h"

namespace lseb {

class Connector {

  uint32_t m_credits;

  uint8_t m_min_rtr_timer;
  uint8_t m_retry_count;
  uint8_t m_rnr_retry_count;

 public:

  Connector(int credits);
  Connector(Connector const& other) = delete;  // non construction-copyable
  Connector& operator=(Connector const&) = delete;  // non copyable
  std::unique_ptr<Socket> connect(
      std::string const& hostname,
      std::string const& port);
};

}

#endif
