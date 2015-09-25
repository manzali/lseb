#ifndef TRANSPORT_TCP_ACCEPTOR_TCP_H
#define TRANSPORT_TCP_ACCEPTOR_TCP_H

#include <type_traits>

#include <boost/asio.hpp>

#include "common/utility.h"

#include "transport/tcp/socket_tcp.h"

namespace lseb {

template<typename T>
class Acceptor {
  boost::asio::io_service m_io_service;
  boost::asio::ip::tcp::acceptor m_acceptor;

 public:
  Acceptor()
      :
        m_io_service(),
        m_acceptor(m_io_service) {
    static_assert(std::is_base_of<Socket, T>::value, "Wrong template");
  }

  void register_memory(void* buffer, size_t size, int credits) {
  }

  void listen(std::string const& hostname, std::string const& port) {
    boost::asio::ip::tcp::endpoint endpoint(
      boost::asio::ip::address::from_string(hostname),
      std::stol(port));
    m_acceptor.open(endpoint.protocol());
    m_acceptor.bind(endpoint);
    m_acceptor.listen();
  }

  T accept() {
    std::unique_ptr<boost::asio::ip::tcp::socket> socket_ptr(
      new boost::asio::ip::tcp::socket(m_io_service));
    m_acceptor.accept(*socket_ptr);
    return T(std::move(socket_ptr));
  }

};

}

#endif
