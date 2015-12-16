#ifndef TRANSPORT_TCP_SOCKET_TCP_H
#define TRANSPORT_TCP_SOCKET_TCP_H

#include <atomic>
#include <queue>

#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include "common/utility.h"

namespace lseb {

class SendSocket {
  std::shared_ptr<boost::asio::ip::tcp::socket> m_socket_ptr;
  int m_pending;
  boost::mutex m_mutex;
  bool m_is_writing;
  std::queue<iovec> m_free_iovec_queue;
  std::queue<iovec> m_full_iovec_queue;
  void async_send(iovec const& iov);

 public:
  SendSocket(std::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr);
  void register_memory(void* buffer, size_t size) {
  }
  std::vector<void*> pop_completed();
  size_t post_send(iovec const& iov);
  int pending();
};

class RecvSocket {
  std::shared_ptr<boost::asio::ip::tcp::socket> m_socket_ptr;
  boost::mutex m_mutex;
  bool m_is_reading;
  std::queue<iovec> m_free_iovec_queue;
  std::queue<iovec> m_full_iovec_queue;
  void async_recv(iovec const& iov);

 public:
  RecvSocket(std::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr);
  void register_memory(void* buffer, size_t size) {
  }
  std::vector<iovec> pop_completed();
  void post_recv(iovec const& iov);
  void post_recv(std::vector<iovec> const& iov_vect);
  std::string peer_hostname();
};

}

#endif
