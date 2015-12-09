#ifndef TRANSPORT_TCP_ACCEPTOR_TCP_H
#define TRANSPORT_TCP_ACCEPTOR_TCP_H

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
class Acceptor {

  boost::asio::io_service m_io_service;
  boost::asio::ip::tcp::acceptor m_acceptor;
  boost::asio::deadline_timer m_timer;
  std::vector<std::thread> m_threads;

 public:
  Acceptor(int credits, int threads = 1)
      :
        m_io_service(),
        m_acceptor(m_io_service),
        m_timer(m_io_service) {
    m_timer.expires_at(boost::posix_time::pos_infin);
    m_timer.async_wait(
      [this](const boost::system::error_code &ec) {std::cout << "TIMER EXPIRED!\n";});
    for (int i = 0; i < threads; ++i) {
      m_threads.push_back(std::thread([&]() {m_io_service.run();}));
    }
  }

  ~Acceptor() {
    m_timer.cancel();
    m_io_service.stop();
    for(auto& t : m_threads) {
      t.join();
    }
  }

  void listen(std::string const& hostname, std::string const& port) {
    boost::asio::ip::tcp::endpoint endpoint(
      boost::asio::ip::address::from_string(hostname),
      std::stol(port));
    m_acceptor.open(endpoint.protocol());
    m_acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    m_acceptor.bind(endpoint);
    m_acceptor.listen();
  }

  std::unique_ptr<T> accept() {
    std::unique_ptr<boost::asio::ip::tcp::socket> socket_ptr(
      new boost::asio::ip::tcp::socket(m_io_service));
    m_acceptor.accept(*socket_ptr);
    std::unique_ptr<T> socket(new T(std::move(socket_ptr)));
    return socket;
  }

};

}

#endif
