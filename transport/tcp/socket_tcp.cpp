#include "transport/tcp/socket_tcp.h"

#include <stdexcept>

#include <boost/array.hpp>

namespace lseb {

SendSocket::SendSocket(std::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr)
    :
      m_socket_ptr(std::move(socket_ptr)),
      m_pending(0),
      m_is_writing(false) {
}

std::vector<iovec> SendSocket::pop_completed() {
  std::vector<iovec> vect;
  // Take lock
  boost::mutex::scoped_lock lock(m_mutex);
  while (!m_full_iovec_queue.empty()) {
    vect.push_back(m_full_iovec_queue.front());
    m_full_iovec_queue.pop();
    m_pending--;
  }
  return vect;
}

void SendSocket::async_send(iovec const& iov) {
  std::vector<boost::asio::const_buffer> buffers;
  buffers.push_back(boost::asio::buffer(&iov.iov_len, sizeof(iov.iov_len)));
  buffers.push_back(boost::asio::buffer(iov.iov_base, iov.iov_len));
  //std::cout << "[" << iov.iov_base << "] async_write...\n";
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

        // Take lock
        boost::mutex::scoped_lock lock(m_mutex);
        if(!m_free_iovec_queue.empty()) {
          iovec iov = m_free_iovec_queue.front();
          m_free_iovec_queue.pop();
          async_send(iov);
        }
        else {
          m_is_writing = false;
        }
        m_full_iovec_queue.push(iov);
    });
}


void SendSocket::post_send(iovec const& iov) {
  // Take lock
  boost::mutex::scoped_lock lock(m_mutex);
  ++m_pending;
  if (!m_is_writing) {
    m_is_writing = true;
    async_send(iov);
  }
  else{
    m_free_iovec_queue.push(iov);
  }
}

int SendSocket::pending() {
  boost::mutex::scoped_lock lock(m_mutex);
  return m_pending;
}

RecvSocket::RecvSocket(std::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr)
    :
      m_socket_ptr(std::move(socket_ptr)),
      m_is_reading(false) {
}

std::vector<iovec> RecvSocket::pop_completed() {
  std::vector<iovec> iov_vect;
  // Take lock
  boost::mutex::scoped_lock lock(m_mutex);
  while (!m_full_iovec_queue.empty()) {
    iov_vect.push_back(m_full_iovec_queue.front());
    m_full_iovec_queue.pop();
  }
  return iov_vect;
}

void RecvSocket::async_recv(iovec const& iov) {
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

        // Take lock
        boost::mutex::scoped_lock lock(m_mutex);
        m_full_iovec_queue.push(*p_iov);
        if(!m_free_iovec_queue.empty()){
          iovec iov = m_free_iovec_queue.front();
          m_free_iovec_queue.pop();
          async_recv(iov);
        }
        else{
          m_is_reading = false;
        }
      });
    });
}

void RecvSocket::post_recv(iovec const& iov) {
  // Take lock
  boost::mutex::scoped_lock lock(m_mutex);
  if (!m_is_reading) {
    m_is_reading = true;
    async_recv(iov);
  }
  else{
    m_free_iovec_queue.push(iov);
  }
}

void RecvSocket::post_recv(std::vector<iovec> const& iov_vect) {
  for (auto const& iov : iov_vect) {
    post_recv(iov);
  }
}

std::string RecvSocket::peer_hostname() {
  return m_socket_ptr->remote_endpoint().address().to_string();
}

}
