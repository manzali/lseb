#ifndef TRANSPORT_VERBS_SOCKET_H
#define TRANSPORT_VERBS_SOCKET_H

#include <map>
#include <vector>
#include <string>

#include <cstring>

#include <infiniband/verbs.h>
#include <rdma/rdma_cma.h>

namespace lseb {

namespace verbs {
static const uint8_t RETRY_COUNT = 7;
static const uint8_t RNR_RETRY_COUNT = 7;
static const uint8_t MIN_RTR_TIMER = 1;
}

class Socket {

  rdma_cm_id* m_cm_id;
  std::vector<ibv_mr*> m_mrs;
  uint32_t m_credits;
  std::map<void*, size_t> m_pending_send;
  std::map<void*, size_t> m_pending_recv;

 public:

  Socket(rdma_cm_id* cm_id, uint32_t credits);
  Socket(Socket const& other) = delete;  // non construction-copyable
  Socket& operator=(Socket const&) = delete;  // non copyable
  ~Socket();

  void register_memory(void* buffer, size_t size);

  std::vector<iovec> poll_completed_send();
  std::vector<iovec> poll_completed_recv();

  void post_send(iovec const& iov);
  void post_recv(iovec const& iov);

  bool available_send();
  bool available_recv();

  std::vector<iovec> pending_send();
  std::vector<iovec> pending_recv();

  std::string peer_hostname();
};

}

#endif
