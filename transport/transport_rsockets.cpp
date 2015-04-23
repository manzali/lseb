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

#include "common/log.h"
#include "common/dataformat.h"
#include "common/utility.h"

void set_rdma_options(int socket) {
  int val = 1;
  rsetsockopt(socket, SOL_RDMA, RDMA_IOMAPSIZE, &val, sizeof val);
  val = 0;
  rsetsockopt(socket, SOL_RDMA, RDMA_INLINE, &val, sizeof val);
}

namespace lseb {

RuConnectionId lseb_connect(std::string const& hostname, long port) {

  addrinfo *res;
  if (getaddrinfo(hostname.c_str(), std::to_string(port).c_str(), NULL, &res)) {
    //LOG(WARNING) << "Error on getaddrinfo: " << strerror(errno);
    throw std::runtime_error(
      "Error on getaddrinfo: " + std::string(strerror(errno)));
  }

  int socket = rsocket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (socket < 0) {
    freeaddrinfo(res);
    //LOG(WARNING) << "Error on rsocket creation: " << strerror(errno);
    throw std::runtime_error(
      "Error on rsocket creation: " + std::string(strerror(errno)));
  }

  set_rdma_options(socket);

  if (rconnect(socket, res->ai_addr, res->ai_addrlen)) {
    rclose(socket);
    freeaddrinfo(res);
    //LOG(WARNING) << "Error on rconnect: " << strerror(errno);
    throw std::runtime_error(
      "Error on rconnect: " + std::string(strerror(errno)));
  }

  LOG(DEBUG) << "Connected to host " << hostname << " on port " << port;

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
    //LOG(WARNING) << "Error on getaddrinfo: " << strerror(errno);
    throw std::runtime_error(
      "Error on getaddrinfo: " + std::string(strerror(errno)));
  }

  int socket = rsocket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (socket < 0) {
    freeaddrinfo(res);
    //LOG(WARNING) << "Error on rsocket creation: " << strerror(errno);
    throw std::runtime_error(
      "Error on rsocket creation: " + std::string(strerror(errno)));
  }

  int val = 1;
  if (rsetsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &val, sizeof val)) {
    rclose(socket);
    freeaddrinfo(res);
    //LOG(WARNING) << "Error on rsetsockopt: " << strerror(errno);
    throw std::runtime_error(
      "Error on rsetsockopt: " + std::string(strerror(errno)));
  }

  if (rbind(socket, res->ai_addr, res->ai_addrlen)) {
    rclose(socket);
    freeaddrinfo(res);
    //LOG(WARNING) << "Error on rbind: " << strerror(errno);
    throw std::runtime_error("Error on rbind: " + std::string(strerror(errno)));
  }

  if (rlisten(socket, 128)) {  // 128 seems to be the max number of waiting sockets in linux
    rclose(socket);
    freeaddrinfo(res);
    //LOG(WARNING) << "Error on rlisten: " << strerror(errno);
    throw std::runtime_error(
      "Error on rlisten: " + std::string(strerror(errno)));
  }

  set_rdma_options(socket);

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
    //LOG(WARNING) << "Error on raccept: " << strerror(errno);
    throw std::runtime_error(
      "Error on raccept: " + std::string(strerror(errno)));
  }

  LOG(DEBUG)
    << "Host "
    << inet_ntoa(cli_addr.sin_addr)
    << " connected on port "
    << ntohs(cli_addr.sin_port);

  set_rdma_options(new_socket);

  return BuConnectionId(new_socket, buffer, len);
}

bool lseb_register(RuConnectionId& conn) {

  // Receiving offset of buffer
  if (rrecv(conn.socket, &conn.offset, sizeof(conn.offset), MSG_WAITALL) == -1) {
    //LOG(WARNING) << "Error on rrecv: " << strerror(errno);
    throw std::runtime_error("Error on rrecv: " + std::string(strerror(errno)));
  }

  // Register poll_byte
  off_t offset = riomap(
    conn.socket,
    (void*) &conn.poll_byte,
    sizeof(conn.poll_byte),
    PROT_WRITE,
    0,
    -1);
  if (offset == -1) {
    //LOG(WARNING) << "Error on riomap: " << strerror(errno);
    throw std::runtime_error(
      "Error on riomap: " + std::string(strerror(errno)));
  }

  // Sending offset of poll_byte
  if (rsend(conn.socket, &offset, sizeof(offset), 0) == -1) {
    LOG(WARNING) << "Error on rsend: " << strerror(errno);
    throw std::runtime_error("Error on rsend: " + std::string(strerror(errno)));
  }

  return true;
}

bool lseb_register(BuConnectionId& conn) {

  // Register buffer
  off_t offset = riomap(
    conn.socket,
    (void*) conn.buffer,
    conn.len,
    PROT_WRITE,
    0,
    -1);
  if (offset == -1) {
    LOG(WARNING) << "Error on riomap: " << strerror(errno);
    throw std::runtime_error(
      "Error on riomap: " + std::string(strerror(errno)));
  }

  // Sending offset of buffer
  if (rsend(conn.socket, &offset, sizeof(offset), MSG_WAITALL) == -1) {
    LOG(WARNING) << "Error on rsend: " << strerror(errno);
    throw std::runtime_error("Error on rsend: " + std::string(strerror(errno)));
  }

  // Receiving offset of poll_byte
  if (rrecv(conn.socket, &conn.offset, sizeof(conn.offset), MSG_WAITALL) == -1) {
    LOG(WARNING) << "Error on rrecv: " << strerror(errno);
    throw std::runtime_error("Error on rrecv: " + std::string(strerror(errno)));
  }

  return true;
}

bool lseb_poll(RuConnectionId& conn) {
  return conn.poll_byte == READY_TO_WRITE;
}

bool lseb_poll(BuConnectionId& conn) {
  if (conn.event_id != 0) {
    uint64_t id =
      static_cast<EventHeader volatile*>(static_cast<void volatile*>(conn
        .buffer))->id;
    return conn.event_id != id;
  }
  uint64_t length =
    static_cast<EventHeader volatile*>(static_cast<void volatile*>(conn.buffer))
      ->length;
  return length != 0;
}

ssize_t lseb_write(RuConnectionId& conn, std::vector<iovec> const& iov) {

  while (!lseb_poll(conn)) {
    ;
  }

  conn.poll_byte = READY_TO_READ;

  size_t length;
  void* buffer;

  if (iov.size() == 1) {
    buffer = iov[0].iov_base;
    length = iov[0].iov_len;
  }
  else{
    length = 0;
    for (iovec const& i : iov) {
      length += i.iov_len;
    }
    uint8_t single_iov[length];
    length = 0;
    for (iovec const& i : iov) {
      memcpy(single_iov + length, i.iov_base, i.iov_len);
      length += i.iov_len;
    }
    buffer = single_iov;
  }

  return riowrite(conn.socket, buffer, length, conn.offset, 0);
}

ssize_t lseb_read(BuConnectionId& conn, size_t events_in_multievent) {

  while (!lseb_poll(conn)) {
    ;
  }

  conn.event_id =
    static_cast<EventHeader volatile*>(static_cast<void volatile*>(conn.buffer))
      ->id;

  ssize_t bytes_read = 0;
  for (size_t i = 0; i < events_in_multievent; ++i) {
    bytes_read +=
      static_cast<EventHeader volatile*>(static_cast<void volatile*>(conn
        .buffer + bytes_read))->length;
  }

  uint8_t poll_byte = READY_TO_WRITE;
  riowrite(
    conn.socket,
    static_cast<void*>(&poll_byte),
    sizeof(poll_byte),
    conn.offset,
    0);

  return bytes_read;  // This is not a real read but just an ack for sender
}

}
