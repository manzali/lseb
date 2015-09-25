#ifndef TRANSPORT_TCP_SOCKET_TCP_H
#define TRANSPORT_TCP_SOCKET_TCP_H

#include <boost/asio.hpp>

#include "common/utility.h"

namespace lseb {

class Socket {
 public:
  Socket() {

  }
};

class SendSocket : public Socket {
  std::unique_ptr<boost::asio::ip::tcp::socket> m_socket_ptr;

 public:
  SendSocket(std::unique_ptr<boost::asio::ip::tcp::socket> socket_ptr);
  int available();
  size_t write(DataIov const& data);
  size_t write_all(std::vector<DataIov> const& data_vect);
};

class RecvSocket : public Socket {
  std::unique_ptr<boost::asio::ip::tcp::socket> m_socket_ptr;
  std::vector<iovec> m_iov_vect;

 public:
  RecvSocket(std::unique_ptr<boost::asio::ip::tcp::socket> socket_ptr);
  iovec read();
  std::vector<iovec> read_all();
  void release(std::vector<iovec> const& iov_vect);
};

}

#endif
