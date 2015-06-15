#include "transport/transport_tcp.h"

#include <algorithm>
#include <stdexcept>

#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "common/log.hpp"
#include "common/utility.h"
#include "common/dataformat.h"

namespace lseb {

RuConnectionId lseb_connect(std::string const& hostname, long port) {
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    throw std::runtime_error(
      "Error on socket creation: " + std::string(strerror(errno)));
  }

  sockaddr_in serv_addr;
  std::fill(
    reinterpret_cast<char*>(&serv_addr),
    reinterpret_cast<char*>(&serv_addr) + sizeof(serv_addr),
    0);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);

  hostent const& server = *(gethostbyname(hostname.c_str()));
  std::copy(server.h_addr,
  server.h_addr + server.h_length,
  reinterpret_cast<char*>(&serv_addr.sin_addr.s_addr)
  );

  if (connect(sockfd, (sockaddr*) &serv_addr, sizeof(serv_addr)) != 0) {
    throw std::runtime_error(
      "Error on connect: " + std::string(strerror(errno)));
  }

  LOG(DEBUG)
    << "Connected to host "
    << inet_ntoa(serv_addr.sin_addr)
    << " on port "
    << port;
  return RuConnectionId(sockfd);
}

BuSocket lseb_listen(std::string const& hostname, long port) {
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    throw std::runtime_error(
      "Error on socket creation: " + std::string(strerror(errno)));
  }

  sockaddr_in serv_addr;
  std::fill(
    reinterpret_cast<char*>(&serv_addr),
    reinterpret_cast<char*>(&serv_addr) + sizeof(serv_addr),
    0);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);

  hostent const& server = *(gethostbyname(hostname.c_str()));
  std::copy(server.h_addr,
  server.h_addr + server.h_length,
  reinterpret_cast<char*>(&serv_addr.sin_addr.s_addr)
  );

  if (bind(sockfd, (sockaddr*) &serv_addr, sizeof(serv_addr))) {
    throw std::runtime_error("Error on bind: " + std::string(strerror(errno)));
  }
  listen(sockfd, 128);  // 128 seems to be the max number of waiting sockets in linux

  LOG(DEBUG)
    << "Host "
    << inet_ntoa(serv_addr.sin_addr)
    << " is listening on port "
    << port;
  return sockfd;
}

BuConnectionId lseb_accept(BuSocket const& socket, void* buffer, size_t len) {
  sockaddr_in cli_addr;
  socklen_t clilen = sizeof(cli_addr);
  int newsockfd = accept(socket, (sockaddr*) &cli_addr, &clilen);
  if (newsockfd == -1) {
    throw std::runtime_error(
      "Error on accept: " + std::string(strerror(errno)));
  }

  LOG(DEBUG)
    << "Host "
    << inet_ntoa(cli_addr.sin_addr)
    << " connected on port "
    << ntohs(cli_addr.sin_port);

  return BuConnectionId(newsockfd, buffer, len);
}

bool lseb_sync(RuConnectionId const& conn) {
  uint8_t poll_byte;
  ssize_t ret = send(conn.socket, &poll_byte, sizeof(poll_byte), 0);
  ret = recv(conn.socket, &poll_byte, sizeof(poll_byte), MSG_WAITALL);
  return ret == sizeof(poll_byte);
}

bool lseb_sync(BuConnectionId const& conn) {
  uint8_t poll_byte;
  ssize_t ret = recv(conn.socket, &poll_byte, sizeof(poll_byte), MSG_WAITALL);
  ret = send(conn.socket, &poll_byte, sizeof(poll_byte), 0);
  return ret == sizeof(poll_byte);
}

bool lseb_poll(RuConnectionId const& conn) {
  pollfd poll_fd { conn.socket, POLLOUT, 0 };
  int ret = poll(&poll_fd, 1, 0);
  if (ret == -1) {
    throw std::runtime_error("Error on poll: " + std::string(strerror(errno)));
  }
  return ret != 0;
}

bool lseb_poll(BuConnectionId const& conn) {
  pollfd poll_fd { conn.socket, POLLIN, 0 };
  int ret = poll(&poll_fd, 1, 0);
  if (ret == -1) {
    throw std::runtime_error("Error on poll: " + std::string(strerror(errno)));
  }
  return ret != 0;
}

ssize_t lseb_write(RuConnectionId const& conn, std::vector<iovec>& iov) {
  size_t length = 0;
  for (iovec const& i : iov) {
    length += i.iov_len;
  }
  // Add as first iovect the length of data
  iov.insert(iov.begin(), { &length, sizeof(length) });
  ssize_t ret = writev(conn.socket, iov.data(), iov.size());
  if (ret == -1) {
    throw std::runtime_error(
      "Error on writev: " + std::string(strerror(errno)));
  }
  iov.erase(iov.begin());
  return ret - sizeof(length);
}

std::vector<iovec> lseb_read(BuConnectionId& conn) {
  // receive the length of data
  ssize_t ret = recv(
    conn.socket,
    static_cast<void*>(&conn.avail),
    sizeof(conn.avail),
    MSG_WAITALL);
  if (ret == -1) {
    throw std::runtime_error("Error on recv: " + std::string(strerror(errno)));
  } else if (ret == 0) {
    // well-known problem (to be investigated)
    throw std::runtime_error("Error on recv: null length received");
  }
  assert(conn.avail <= conn.len && "Too much incoming data");
  ret = recv(conn.socket, conn.buffer, conn.avail, MSG_WAITALL);
  if (ret == -1) {
    throw std::runtime_error("Error on recv: " + std::string(strerror(errno)));
  }

  std::vector<iovec> iov;
  iov.push_back( { conn.buffer, conn.avail });

  return iov;
}

void lseb_release(BuConnectionId& conn) {

}

}