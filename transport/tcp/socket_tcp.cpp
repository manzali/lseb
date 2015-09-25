#include "transport/tcp/socket_tcp.h"

#include <stdexcept>

#include <boost/array.hpp>

namespace lseb {

SendSocket::SendSocket(std::unique_ptr<boost::asio::ip::tcp::socket> socket_ptr)
    :
      m_socket_ptr(std::move(socket_ptr)) {

}

int SendSocket::available() {
  return 1;
}

size_t SendSocket::write(DataIov const& data) {
  size_t bytes = 0;
  std::vector<boost::asio::const_buffer> buffers;
  for (auto& iov : data) {
    bytes += iov.iov_len;
    buffers.push_back(boost::asio::buffer(iov.iov_base, iov.iov_len));
  }
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
  boost::asio::write(
    *m_socket_ptr,
    buffers,
    boost::asio::transfer_all(),
    error);
  if (error) {
    std::cout << "error on write 2\n";
    throw boost::system::system_error(error);
  }
  return bytes;
}

size_t SendSocket::write_all(std::vector<DataIov> const& data_vect) {
  size_t bytes = 0;
  for (auto& data : data_vect) {
    bytes += write(data);
  }
  return bytes;
}

RecvSocket::RecvSocket(std::unique_ptr<boost::asio::ip::tcp::socket> socket_ptr)
    :
      m_socket_ptr(std::move(socket_ptr)) {

}

iovec RecvSocket::read() {
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

std::vector<iovec> RecvSocket::read_all() {
  std::vector<iovec> iov_vect;
  boost::asio::socket_base::bytes_readable command(true);
  m_socket_ptr->io_control(command);
  size_t bytes_readable = command.get();
  while (bytes_readable && m_iov_vect.size()) {
    iov_vect.push_back(read());
    m_socket_ptr->io_control(command);
    bytes_readable = command.get();
  }
  return iov_vect;
}

void RecvSocket::release(std::vector<iovec> const& iov_vect) {
  m_iov_vect.insert(m_iov_vect.end(), iov_vect.begin(), iov_vect.end());
}

}
