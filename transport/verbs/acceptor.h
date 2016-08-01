#ifndef TRANSPORT_VERBS_ACCEPTOR_H
#define TRANSPORT_VERBS_ACCEPTOR_H

#include <memory>

#include <rdma/rdma_cma.h>

#include "transport/verbs/socket.h"

namespace lseb {

class Acceptor {

  uint32_t m_credits;

  uint8_t m_min_rtr_timer;
  uint8_t m_rnr_retry_count;

  rdma_cm_id* m_cm_id;

 public:

  Acceptor(int credits);
  Acceptor(Acceptor const& other) = delete;  // non construction-copyable
  Acceptor& operator=(Acceptor const&) = delete;  // non copyable
  void listen(std::string const& hostname, std::string const& port);
  ~Acceptor();
  std::unique_ptr<Socket> accept();
};

}

#endif
