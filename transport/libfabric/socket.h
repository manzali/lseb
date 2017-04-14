#ifndef TRANSPORT_VERBS_SOCKET_H
#define TRANSPORT_VERBS_SOCKET_H

#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include <tuple>

#include <cstdint>

#include <rdma/fabric.h>
#include "shared.h"

namespace lseb {
#ifdef FI_VERBS
struct MemoryRegion {
  fabric_ptr<fid_mr> mr = nullptr;
  unsigned char* begin;
  unsigned char* end;
  MemoryRegion(void* buffer, size_t size);
};
#endif

class Socket {
 public:
#ifdef FI_VERBS
  typedef MemoryRegion memory_region;
#endif
  Socket(fid_ep* ep, fid_cq* rx_cq, fid_cq* tx_cq, uint32_t credits);
  Socket(Socket const& other) = delete;
  Socket &operator=(Socket const&) = delete;
  Socket(Socket &&other) = default;
  Socket &operator=(Socket &&) = default;
  ~Socket() = default;

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

 private:
  // NOTE: Declarations sorted by deconstruction requirements

  // Endpoint
  fabric_ptr<fid_ep> m_ep;

  // Completion Queues
  fabric_ptr<fid_cq> m_rx_cq;  // Receive CQ
  fabric_ptr<fid_cq> m_tx_cq;  // Transmit CQ

  uint32_t m_credits;  // CQ Depth
#ifdef FI_VERBS
  std::vector<memory_region> m_mrs;
#endif
  // TODO: Remove the unordered_map because we let the application store the
  //       length of the registered buffers. Maybe an unordered_set is needed
  std::unordered_map<void*, size_t> m_pending_send;
  std::unordered_map<void*, size_t> m_pending_recv;

  // Used as temporary buffer for reading completions from rx/tx queues
  std::vector<fi_cq_entry> m_comp_send;
  std::vector<fi_cq_entry> m_comp_recv;

};

}
#endif
