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
//std::cout << "[" << iov.iov_base << "] async_write...\n";
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
      //std::cout << "[" << iov.iov_base << "] async_write: sent " << byte_transferred << " bytes\n";
      m_shared_queue.push(iov);
    });

  return iov.iov_len;
}

int SendSocket::pending() {
  return m_pending.load();
}

RecvSocket::RecvSocket(std::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr)
    :
      m_socket_ptr(std::move(socket_ptr)),
      m_is_reading(0) {
}

std::vector<iovec> RecvSocket::pop_completed() {
  std::vector<iovec> iov_vect;
  std::pair<iovec, bool> p = m_full_shared_queue.pop_no_wait();
  while (p.second) {
    iov_vect.push_back(p.first);
    p = m_full_shared_queue.pop_no_wait();
  }
  return iov_vect;
}

void RecvSocket::async_read(iovec const& iov) {
  m_is_reading++; // missing --
  std::shared_ptr<iovec> p_iov(new iovec);
  p_iov->iov_base = iov.iov_base;
    boost::array<boost::asio::mutable_buffer, 2> buffers = { boost::asio::buffer(
    &(p_iov->iov_len),
    sizeof(p_iov->iov_len)), boost::asio::buffer(
    boost::asio::buffer(p_iov->iov_base, iov.iov_len)) };
  //std::cout << "[" << p_iov->iov_base << "] async_read (at least " << sizeof(p_iov->iov_len) << " bytes)...\n";
  boost::asio::async_read(
    *m_socket_ptr,
    buffers,
    boost::asio::transfer_at_least(sizeof(p_iov->iov_len)),
    [this, p_iov](boost::system::error_code const& error, size_t byte_transferred) {
      if(error) {
        std::cout << "Error on async_read: " << boost::system::system_error(error).what() << std::endl;
        throw boost::system::system_error(error);
      }
      //std::cout << "[" << p_iov->iov_base << "] async_read: received " << byte_transferred << " bytes\n";

      assert(p_iov->iov_len > 0);
      assert(byte_transferred >= sizeof(p_iov->iov_len));
      byte_transferred -= sizeof(p_iov->iov_len);
      int remain = p_iov->iov_len - byte_transferred;
      assert(remain >= 0);

      //std::cout << "[" << p_iov->iov_base << "] async_read (exactly " << remain << " bytes)...\n";
      boost::asio::async_read(
        *m_socket_ptr,
        boost::asio::buffer(static_cast<char*>(p_iov->iov_base) + byte_transferred, remain),
        boost::asio::transfer_all(),
        [this, p_iov](boost::system::error_code const& error, size_t byte_transferred) {
        if(error) {
          std::cout << "Error on async_read: " << boost::system::system_error(error).what() << std::endl;
          throw boost::system::system_error(error);
        }
        //std::cout << "[" << p_iov->iov_base << "] async_read: received " << byte_transferred << " bytes\n";
        m_full_shared_queue.push(*p_iov);

        std::pair<iovec, bool> p = m_empty_shared_queue.pop_no_wait();
        if(p.second){
          async_read(p.first);
        }
        m_is_reading--;
      });
    });
}

void RecvSocket::post_read(iovec const& iov) {
  m_empty_shared_queue.push(iov);
  if(m_is_reading.load() == 0){
    std::pair<iovec, bool> p = m_empty_shared_queue.pop_no_wait();
    if(p.second){
      async_read(p.first);
    }
  }
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
