#ifndef TRANSPORT_TCP_SOCKET_TCP_H
#define TRANSPORT_TCP_SOCKET_TCP_H

#include <atomic>

#include <boost/asio.hpp>

#include "common/utility.h"
#include "transport/tcp/shared_queue.h"

namespace lseb {

class SendSocket {
  std::shared_ptr<boost::asio::ip::tcp::socket> m_socket_ptr;
  SharedQueue<iovec> m_shared_queue;
  std::atomic<int> m_pending;

 public:
  SendSocket(std::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr);
  void register_memory(void* buffer, size_t size) {
  }
  std::vector<void*> pop_completed();
  size_t post_write(iovec const& iov);
  int pending();
};

class RecvSocket {
  std::shared_ptr<boost::asio::ip::tcp::socket> m_socket_ptr;
  SharedQueue<iovec> m_shared_queue;

 public:
  RecvSocket(std::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr);
  void register_memory(void* buffer, size_t size) {
  }
  std::vector<iovec> pop_completed();
  void post_read(iovec const& iov);
  void post_read(std::vector<iovec> const& iov_vect);
  std::string peer_hostname();
};

}

#endif
