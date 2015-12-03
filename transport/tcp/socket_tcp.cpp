#include "transport/tcp/socket_tcp.h"

#include <stdexcept>

#include <boost/array.hpp>

namespace lseb {

SendSocket::SendSocket(std::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr)
    :
      m_socket_ptr(std::move(socket_ptr)),
      m_pending(0) {
}

std::vector<void*> SendSocket::pop_completed() {
  std::vector<void*> vect;
  std::pair<iovec, bool> p = m_shared_queue.pop_no_wait();
  while (p.second) {
    m_pending--;
    vect.push_back(p.first.iov_base);
    p = m_shared_queue.pop_no_wait();
  }
  return vect;
}

size_t SendSocket::post_write(iovec const& iov) {
  std::vector<boost::asio::const_buffer> buffers;
  buffers.push_back(boost::asio::buffer(&iov.iov_len, sizeof(iov.iov_len)));
  buffers.push_back(boost::asio::buffer(iov.iov_base, iov.iov_len));
  //std::cout << "async_write...\n";
  m_pending++;
  boost::asio::async_write(
    *m_socket_ptr,
    buffers,
    [this, iov](boost::system::error_code const& error, size_t byte_transferred) {
      if(error) {
        std::cout << "Error on async_write: " << boost::system::system_error(error).what() << std::endl;
        throw boost::system::system_error(error);
      }
      assert(byte_transferred >= sizeof(iov.iov_len));
      assert(iov.iov_len == byte_transferred - sizeof(iov.iov_len));
      //std::cout << "async_write: byte_transferred " << byte_transferred << std::endl;
      m_shared_queue.push(iov);
    });

  return iov.iov_len;
}

int SendSocket::pending() {
  return m_pending.load();
}

RecvSocket::RecvSocket(std::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr)
    :
      m_socket_ptr(std::move(socket_ptr)) {
}

std::vector<iovec> RecvSocket::pop_completed() {
  std::vector<iovec> iov_vect;
  std::pair<iovec, bool> p = m_shared_queue.pop_no_wait();
  while (p.second) {
    iov_vect.push_back(p.first);
    p = m_shared_queue.pop_no_wait();
  }
  return iov_vect;
}

void RecvSocket::post_read(iovec const& iov) {
  std::shared_ptr<size_t> p_len(new size_t);
  boost::array<boost::asio::mutable_buffer, 2> buffers = { boost::asio::buffer(
    p_len.get(),
    sizeof(*p_len)), boost::asio::buffer(
    boost::asio::buffer(iov.iov_base, iov.iov_len)) };
  //std::cout << "async_read...\n";
  boost::asio::async_read(
    *m_socket_ptr,
    buffers,
    boost::asio::transfer_at_least(sizeof(*p_len)),
    [this, p_len, iov](boost::system::error_code const& error, size_t byte_transferred) {
      if(error) {
        std::cout << "Error on async_read: " << boost::system::system_error(error).what() << std::endl;
        throw boost::system::system_error(error);
      }
      //std::cout << "async_read: byte_transferred " << byte_transferred << std::endl;

      size_t len = *p_len;
      assert(len <= iov.iov_len);
      assert(byte_transferred >= sizeof(len));
      byte_transferred -= sizeof(len);
      int remain = len - byte_transferred;
      assert(remain >= 0);
      if(remain) {
        boost::system::error_code error2;
        size_t byte_transferred2 = boost::asio::read(
            *m_socket_ptr,
            boost::asio::buffer(static_cast<char*>(iov.iov_base) + byte_transferred, remain),
            boost::asio::transfer_all(),
            error2);
        //std::cout << "read: byte_transferred " << byte_transferred2 << std::endl;
        assert(remain == byte_transferred2);
        if(error2) {
          std::cout << "Error on read: " << boost::system::system_error(error2).what() << std::endl;
          throw boost::system::system_error(error2);
        }
      }
      m_shared_queue.push( {iov.iov_base, len});
    });
}

void RecvSocket::post_read(std::vector<iovec> const& iov_vect) {
  for (auto const& iov : iov_vect) {
    post_read(iov);
  }
}

std::string RecvSocket::peer_hostname() {
  return m_socket_ptr->remote_endpoint().address().to_string();
}

}
