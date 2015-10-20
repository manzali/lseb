#ifndef TRANSPORT_TCP_CONNECTOR_TCP_H
#define TRANSPORT_TCP_CONNECTOR_TCP_H

#include <type_traits>

#include <boost/asio.hpp>

#include "common/utility.h"

#include "transport/tcp/socket_tcp.h"

namespace lseb {

template<typename T>
class Connector {
  static_assert(std::is_base_of<Socket, T>::value, "Wrong template");

  boost::asio::io_service m_io_service;

 public:
  Connector(void* buffer, size_t size, int credits) {
  }

  T connect(std::string const& hostname, std::string const& port) {
    boost::asio::ip::tcp::resolver resolver(m_io_service);
    boost::asio::ip::tcp::resolver::query query(hostname, port);
    boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);
    boost::asio::ip::tcp::resolver::iterator end;
    std::unique_ptr<boost::asio::ip::tcp::socket> socket_ptr(
      new boost::asio::ip::tcp::socket(m_io_service));
    boost::system::error_code error = boost::asio::error::host_not_found;
    while (error && iterator != end) {
      socket_ptr->close();
      socket_ptr->connect(*iterator, error);
      if (error == boost::asio::error::connection_refused) {
        throw boost::system::system_error(error);  // Connection refused
      } else {
        iterator++;
      }
    }
    if (error) {
      throw boost::system::system_error(error);
    }
    return T(std::move(socket_ptr));
  }
};

}

#endif
