#ifndef COMMON_TCP_BARRIER_H
#define COMMON_TCP_BARRIER_H

#include <chrono>
#include <string>

#include <boost/array.hpp>
#include <boost/asio.hpp>

#include <cassert>

namespace lseb {

namespace {
auto retry_wait = std::chrono::milliseconds(500);
}

void tcp_barrier(
  int id,
  int size,
  std::string const& hostname,
  std::string const& port) {
  assert(id >= 0 && size >= 0);
  assert(id < size);
  if (id == 0) {
    boost::asio::io_service io_service;
    boost::asio::ip::tcp::acceptor acceptor(
      io_service,
      boost::asio::ip::tcp::endpoint(
        boost::asio::ip::tcp::v4(),
        std::stol(port)));
    std::vector<boost::asio::ip::tcp::socket> sockets;
    for (int i = 1; i < size; ++i) {
      sockets.emplace_back(io_service);
      acceptor.accept(sockets.back());
    }
  } else {
    boost::asio::io_service io_service;
    boost::asio::ip::tcp::resolver resolver(io_service);
    boost::asio::ip::tcp::resolver::query query(hostname.c_str(), port.c_str());
    boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver
      .resolve(query);
    boost::asio::ip::tcp::resolver::iterator end;
    boost::asio::ip::tcp::socket socket(io_service);
    boost::system::error_code error = boost::asio::error::host_not_found;
    // First iteration
    while (error && endpoint_iterator != end) {
      socket.close();
      socket.connect(*endpoint_iterator, error);
      if (error == boost::asio::error::connection_refused) {
        std::this_thread::sleep_for(retry_wait);
      } else {
        endpoint_iterator++;
      }
    }
    if (error) {
      throw boost::system::system_error(error);
    }
    while (true) {
      int dummy;
      boost::system::error_code error;
      socket.read_some(boost::asio::buffer(&dummy, sizeof(dummy)), error);
      if (error == boost::asio::error::eof) {
        break;
      } else if (error) {
        throw boost::system::system_error(error);
      }
    }
  }
}

}

#endif
