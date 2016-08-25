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

class Socket {
public:

  typedef std::unique_ptr<fid_mr, fid_deleter<fid_mr>> mr_ptr;
  typedef std::pair<void*, size_t> mr_info;
  typedef std::pair<mr_ptr, mr_info> memory_region;

  Socket(fid_ep *ep,
         fid_cq *rx_cq,
         fid_cq *tx_cq,
         uint32_t credits);
  Socket(Socket const &other) = delete;
  Socket &operator=(Socket const &) = delete;
  Socket(Socket &&other) = default;
  Socket &operator=(Socket &&) = default;
  ~Socket() = default;

  void register_memory(void *buffer, size_t size);

  std::vector<iovec> poll_completed_send();
  std::vector<iovec> poll_completed_recv();

  void post_send(iovec const &iov);
  void post_recv(iovec const &iov);

  bool available_send();
  bool available_recv();

  std::vector<iovec> pending_send();
  std::vector<iovec> pending_recv();

  std::string peer_hostname();

private:

  // TODO: Decide ownership of these resources
  // TODO: Order of declaration is important in deconstruction: order members

  /* Endpoint
   * Allocated by c/a, owned by socket
   * TODO: Use unique_ptr
   */
  fid_ep *m_ep;

  /* Completion Queues:
   * Allocated and owned by socket
   * TODO: Use unique_ptr
   * Note: An endpoint that will generate asynchronous completions, either
   * through data transfer operations or communication establishment events,
   * must be bound to the appropriate completion queues or event queues before
   * being enabled.
   * TODO: Bind before ep activation
   * Note: The fi_connect and fi_accept calls will also transition an endpoint into
   * the enabled state, if it is not already active.
   */
  fid_cq *m_rx_cq; // Receive CQ
  fid_cq *m_tx_cq; // Transmit CQ

  uint32_t m_credits; // CQ Depth
  std::vector<memory_region> m_mrs;
  // TODO: Remove the unordered_map because we let the application store the
  //       length of the registered buffers. Maybe an unordered_set is needed
  std::unordered_map<void *, size_t> m_pending_send;
  std::unordered_map<void *, size_t> m_pending_recv;

};

}
#endif
