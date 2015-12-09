#ifndef TRANSPORT_TCP_CONNECTOR_TCP_H
#define TRANSPORT_TCP_CONNECTOR_TCP_H

#include <type_traits>
#include <vector>
#include <thread>
#include <chrono>

#include <boost/asio.hpp>
#include <boost/asio/deadline_timer.hpp>

#include "common/utility.h"

#include "transport/tcp/socket_tcp.h"

namespace lseb {

template<typename T>
class Connector {

  boost::asio::io_service m_io_service;
  boost::asio::deadline_timer m_timer;
  std::vector<std::thread> m_threads;

 public:
  Connector(int credits, int threads = 1):
  m_io_service(),
  m_timer(m_io_service) {
    m_timer.expires_at(boost::posix_time::pos_infin);
    m_timer.async_wait(
      [this](const boost::system::error_code &ec) {std::cout << "TIMER EXPIRED!\n";});
    for (int i = 0; i < threads; ++i) {
      m_threads.push_back(std::thread([&]() {m_io_service.run();}));
    }
  }

  ~Connector() {
    m_timer.cancel();
    m_io_service.stop();
    for(auto& t : m_threads) {
      t.join();
    }
  }

  std::unique_ptr<T> connect(std::string const& hostname, std::string const& port) {
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
        ++iterator;
      }
    }
    if (error) {
      throw boost::system::system_error(error);
    }
    std::unique_ptr<T> socket(new T(std::move(socket_ptr)));
    return socket;
  }
};

}

#endif
