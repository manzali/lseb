#include "transport/transport_rsockets.h"

#include <algorithm>
#include <stdexcept>

#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <errno.h>

#include <rdma/rsocket.h>

#include "common/utility.h"

void set_rdma_options(int socket, int iomap_val) {
  rsetsockopt(socket, SOL_RDMA, RDMA_IOMAPSIZE, &iomap_val, sizeof iomap_val);
  int val = 0;
  rsetsockopt(socket, SOL_RDMA, RDMA_INLINE, &val, sizeof val);
}

namespace lseb {

RuConnectionId lseb_connect(std::string const& hostname, long port) {

  addrinfo *res;
  if (getaddrinfo(hostname.c_str(), std::to_string(port).c_str(), NULL, &res)) {
    throw std::runtime_error(
      "Error on getaddrinfo: " + std::string(strerror(errno)));
  }

  int socket = rsocket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (socket < 0) {
    freeaddrinfo(res);
    throw std::runtime_error(
      "Error on rsocket creation: " + std::string(strerror(errno)));
  }

  set_rdma_options(socket, 2);

  if (rconnect(socket, res->ai_addr, res->ai_addrlen)) {
    rclose(socket);
    freeaddrinfo(res);
    throw std::runtime_error(
      "Error on rconnect: " + std::string(strerror(errno)));
  }

  freeaddrinfo(res);
  return RuConnectionId(socket);
}

BuSocket lseb_listen(std::string const& hostname, long port) {
  addrinfo hints, *res;
  std::fill(
    reinterpret_cast<char*>(&hints),
    reinterpret_cast<char*>(&hints) + sizeof(hints),
    0);

  hints.ai_flags = AI_PASSIVE;
  if (getaddrinfo(NULL, std::to_string(port).c_str(), &hints, &res)) {
    throw std::runtime_error(
      "Error on getaddrinfo: " + std::string(strerror(errno)));
  }

  int socket = rsocket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (socket < 0) {
    freeaddrinfo(res);
    throw std::runtime_error(
      "Error on rsocket creation: " + std::string(strerror(errno)));
  }

  int val = 1;
  if (rsetsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &val, sizeof val)) {
    rclose(socket);
    freeaddrinfo(res);
    throw std::runtime_error(
      "Error on rsetsockopt: " + std::string(strerror(errno)));
  }

  if (rbind(socket, res->ai_addr, res->ai_addrlen)) {
    rclose(socket);
    freeaddrinfo(res);
    throw std::runtime_error("Error on rbind: " + std::string(strerror(errno)));
  }

  if (rlisten(socket, 128)) {  // 128 seems to be the max number of waiting sockets in linux
    rclose(socket);
    freeaddrinfo(res);
    throw std::runtime_error(
      "Error on rlisten: " + std::string(strerror(errno)));
  }

  set_rdma_options(socket, 1);

  freeaddrinfo(res);
  return socket;
}

BuConnectionId lseb_accept(BuSocket const& socket, void* buffer, size_t len) {
  sockaddr_in cli_addr;
  socklen_t clilen = sizeof(cli_addr);
  int new_socket = raccept(
    socket,
    static_cast<sockaddr*>(static_cast<void*>(&cli_addr)),
    &clilen);
  if (new_socket == -1) {
    throw std::runtime_error(
      "Error on raccept: " + std::string(strerror(errno)));
  }

  set_rdma_options(new_socket, 1);

  return BuConnectionId(new_socket, buffer, len);
}

bool lseb_sync(RuConnectionId& conn) {

  // Receiving length of buffer
  if (rrecv(conn.socket, &conn.buffer_len, sizeof(conn.buffer_len),
  MSG_WAITALL) == -1) {
    throw std::runtime_error("Error on rrecv: " + std::string(strerror(errno)));
  }

  // Receiving offset of buffer
  if (rrecv(conn.socket, &conn.buffer_offset, sizeof(conn.buffer_offset),
  MSG_WAITALL) == -1) {
    throw std::runtime_error("Error on rrecv: " + std::string(strerror(errno)));
  }

  // Receiving offset of avail
  if (rrecv(conn.socket, &conn.avail_offset, sizeof(conn.avail_offset),
  MSG_WAITALL) == -1) {
    throw std::runtime_error("Error on rrecv: " + std::string(strerror(errno)));
  }

  // Register poll
  off_t offset = riomap(
    conn.socket,
    (void*) &conn.poll,
    sizeof(conn.poll),
    PROT_WRITE,
    0,
    -1);
  if (offset == -1) {
    throw std::runtime_error(
      "Error on riomap: " + std::string(strerror(errno)));
  }

  // Sending offset of poll
  if (rsend(conn.socket, &offset, sizeof(offset), 0) == -1) {
    throw std::runtime_error("Error on rsend: " + std::string(strerror(errno)));
  }

  return true;
}

bool lseb_sync(BuConnectionId& conn) {

  // Register iomap for buffer
  off_t offset_buffer = riomap(
    conn.socket,
    (void*) conn.buffer,
    conn.buffer_len,
    PROT_WRITE,
    0,
    -1);
  if (offset_buffer == -1) {
    throw std::runtime_error(
      "Error on riomap: " + std::string(strerror(errno)));
  }

  // Register iomap for avail
  // a shared_ptr is needed in order to avoid memory leak
  conn.avail = new size_t();
  off_t offset_avail = riomap(
    conn.socket,
    (void*) conn.avail,
    sizeof(size_t),
    PROT_WRITE,
    0,
    -1);
  if (offset_avail == -1) {
    throw std::runtime_error(
      "Error on riomap: " + std::string(strerror(errno)));
  }

  // Split buffer in two parts
  conn.buffer_len /= 2;

  // Sending length of buffer
  if (rsend(conn.socket, &conn.buffer_len, sizeof(conn.buffer_len), MSG_WAITALL) == -1) {
    throw std::runtime_error("Error on rsend: " + std::string(strerror(errno)));
  }

  // Sending offset of buffer
  if (rsend(conn.socket, &offset_buffer, sizeof(offset_buffer), MSG_WAITALL) == -1) {
    throw std::runtime_error("Error on rsend: " + std::string(strerror(errno)));
  }

  // Sending offset of avail
  if (rsend(conn.socket, &offset_avail, sizeof(offset_avail), MSG_WAITALL) == -1) {
    throw std::runtime_error("Error on rsend: " + std::string(strerror(errno)));
  }

  // Receiving offset of poll
  if (rrecv(conn.socket, &conn.poll_offset, sizeof(conn.poll_offset),
  MSG_WAITALL) == -1) {
    throw std::runtime_error("Error on rrecv: " + std::string(strerror(errno)));
  }

  return true;
}

bool lseb_poll(RuConnectionId& conn) {
  return !conn.is_full || conn.poll != LOCKED;
}

bool lseb_poll(BuConnectionId& conn) {
  return *(conn.avail) != 0;
}

ssize_t lseb_write(RuConnectionId& conn, std::vector<iovec>& iov) {

  while (!lseb_poll(conn)) {
    ;
  }

  if (conn.is_full) {
    if (conn.poll != LOCKED) {
      conn.poll = LOCKED;
      conn.is_first = !conn.is_first;
      size_t ret = riowrite(
        conn.socket,
        &conn.buffer_written,
        sizeof(conn.buffer_written),
        conn.avail_offset,
        0);
      if (ret != sizeof(conn.buffer_written)) {
        throw std::runtime_error(
          "Error on riowrite: " + std::string(strerror(errno)));
      }
      conn.buffer_written = 0;
      conn.is_full = false;
    }
    return -2;
  }

  size_t length = 0;
  for (iovec const& i : iov) {
    length += i.iov_len;
  }

  if (length > conn.buffer_len - conn.buffer_written) {
    if (!conn.buffer_written) {
      throw std::runtime_error("Error on riowrite:  buffer size too small");
    }
    conn.is_full = true;
    return -2;
  }

  for (iovec const& i : iov) {
    size_t ret = riowrite(
      conn.socket,
      i.iov_base,
      i.iov_len,
      conn.buffer_offset + conn.buffer_written + (
          conn.is_first ? 0 : conn.buffer_len),
      0);
    if (ret != i.iov_len) {
      throw std::runtime_error(
        "Error on riowrite: " + std::string(strerror(errno)));
    }
    conn.buffer_written += ret;
  }

  return length;
}

std::vector<iovec> lseb_read(BuConnectionId& conn) {

  while (!lseb_poll(conn)) {
    ;
  }

  std::vector<iovec> iov;
  iov.push_back(
    { static_cast<char*>((void*) conn.buffer) + (
        conn.is_first ? 0 : conn.buffer_len), *(conn.avail) });

  *(conn.avail) = 0;
  conn.is_done = true;
  return iov;
}

void lseb_release(BuConnectionId& conn) {

  if (conn.is_done) {
    conn.is_done = false;
    uint8_t poll = FREE;
    size_t ret = riowrite(
      conn.socket,
      static_cast<void*>(&poll),
      sizeof(poll),
      conn.poll_offset,
      0);
    conn.is_first = !conn.is_first;
    if (ret != sizeof(poll)) {
      throw std::runtime_error(
        "Error on riowrite: " + std::string(strerror(errno)));
    }
  }
}

}
