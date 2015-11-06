#include "transport/tcp/socket_tcp.h"

#include <stdexcept>

#include <boost/array.hpp>

namespace lseb {

SendSocket::SendSocket(std::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr)
    :
      m_socket_ptr(std::move(socket_ptr)) {
}

void SendSocket::register_memory(void* buffer, size_t size) {
}

std::vector<iovec> SendSocket::pop_completed() {
  std::vector<iovec> vect(m_iov_vect);
  m_iov_vect.clear();
  return vect;
}

size_t SendSocket::post_write(iovec const& iov) {
  size_t bytes = iov.iov_len;
  boost::system::error_code error;
  boost::asio::write(
    *m_socket_ptr,
    boost::asio::buffer(&bytes, sizeof(bytes)),
    boost::asio::transfer_all(),
    error);
  if (error) {
    std::cout << "error on write 1\n";
    throw boost::system::system_error(error);
  }
  std::vector<boost::asio::const_buffer> buffers;
  buffers.push_back(boost::asio::buffer(iov.iov_base, iov.iov_len));
  boost::asio::write(
    *m_socket_ptr,
    buffers,
    boost::asio::transfer_all(),
    error);
  if (error) {
    std::cout << "error on write 2\n";
    throw boost::system::system_error(error);
  }
  m_iov_vect.push_back(iov);
  return bytes;
}

int SendSocket::pending() {
  return m_iov_vect.size();
}

RecvSocket::RecvSocket(std::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr)
    :
      m_socket_ptr(std::move(socket_ptr)) {
}

void RecvSocket::register_memory(void* buffer, size_t size) {
}

iovec RecvSocket::read_iov() {
  assert(m_iov_vect.size());
  iovec iov = m_iov_vect.back();
  m_iov_vect.pop_back();
  boost::system::error_code error;
  size_t bytes;
  boost::asio::read(
    *m_socket_ptr,
    boost::asio::buffer(&bytes, sizeof(bytes)),
    boost::asio::transfer_all(),
    error);
  if (error) {
    std::cout << "error on read 1\n";
    throw boost::system::system_error(error);
  }
  boost::asio::read(
    *m_socket_ptr,
    boost::asio::buffer(iov.iov_base, bytes),
    boost::asio::transfer_all(),
    error);
  if (error) {
    std::cout << "error on read 2\n";
    throw boost::system::system_error(error);
  }
  iov.iov_len = bytes;
  return iov;
}

std::vector<iovec> RecvSocket::pop_completed() {
  std::vector<iovec> iov_vect;
  boost::asio::socket_base::bytes_readable command(true);
  m_socket_ptr->io_control(command);
  while (command.get() && !m_iov_vect.empty()) {
    iov_vect.push_back(read_iov());
    m_socket_ptr->io_control(command);
  }
  return iov_vect;
}

void RecvSocket::post_read(iovec const& iov) {
  m_iov_vect.push_back(iov);
}

void RecvSocket::post_read(std::vector<iovec> const& iov_vect) {
  m_iov_vect.insert(m_iov_vect.end(), iov_vect.begin(), iov_vect.end());
}

std::string RecvSocket::peer_hostname() {
  return m_socket_ptr->remote_endpoint().address().to_string();
}

}
