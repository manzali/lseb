#include "transport/transport_rsockets.h"

#include <algorithm>

#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "common/log.h"
#include <rdma/rsocket.h>

namespace lseb {

RuConnectionId lseb_connect(std::string const& hostname, long port) {
  int sockfd = rsocket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    LOG(WARNING) << "Error on rsocket creation: " << strerror(errno);
    return RuConnectionId(sockfd, 0);
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

  if (rconnect(sockfd, (sockaddr*) &serv_addr, sizeof(serv_addr)) != 0) {
    LOG(WARNING) << "Error on rconnect: " << strerror(errno);
    return RuConnectionId(sockfd, 0);
  }

  int val = 1;
  rsetsockopt(
    sockfd,
    SOL_RDMA,
    RDMA_IOMAPSIZE,
    static_cast<void*>(&val),
    sizeof(val));

  // Offset stuff
  off_t offset;
  if (rrecv(sockfd, &offset, sizeof(offset), MSG_WAITALL) == -1) {
    LOG(WARNING) << "Error on rrecv: " << strerror(errno);
    return RuConnectionId(sockfd, 0);
  }

  LOG(DEBUG)
    << "Connected to host "
    << inet_ntoa(serv_addr.sin_addr)
    << " on port "
    << port;
  return RuConnectionId(sockfd, offset);
}

BuSocket lseb_listen(std::string const& hostname, long port) {
  int sockfd = rsocket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    LOG(WARNING) << "Error on rsocket creation: " << strerror(errno);
    return sockfd;
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

  if (rbind(sockfd, (sockaddr*) &serv_addr, sizeof(serv_addr))) {
    LOG(WARNING) << "Error on rbind: " << strerror(errno);
    return sockfd;
  }
  rlisten(sockfd, 128);  // 128 seems to be the max number of waiting sockets in linux

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
  int newsockfd = raccept(socket, (sockaddr*) &cli_addr, &clilen);
  if (newsockfd == -1) {
    LOG(WARNING) << "Error on raccept: " << strerror(errno);
    return BuConnectionId(newsockfd, buffer, len);
  }

  int val = 1;
  rsetsockopt(
    newsockfd,
    SOL_RDMA,
    RDMA_IOMAPSIZE,
    static_cast<void*>(&val),
    sizeof(val));

  // Offset stuff
  off_t offset = riomap(newsockfd, buffer, len, PROT_WRITE, 0, -1);
  if (rsend(newsockfd, &offset, sizeof(offset), MSG_WAITALL) == -1) {
    LOG(WARNING) << "Error on rsend: " << strerror(errno);
    return BuConnectionId(newsockfd, buffer, len);
  }

  LOG(DEBUG)
    << "Host "
    << inet_ntoa(cli_addr.sin_addr)
    << " connected on port "
    << ntohs(cli_addr.sin_port);

  return BuConnectionId(newsockfd, buffer, len);
}

int lseb_poll(std::vector<pollfd>& poll_fds, int timeout_ms) {
  int ret = rpoll(poll_fds.data(), poll_fds.size(), timeout_ms);
  if (ret == -1) {
    LOG(WARNING) << "Error on rpoll: " << strerror(errno);
  }
  return ret;
}

ssize_t lseb_write(RuConnectionId const& conn, std::vector<iovec> const& iov) {
  ssize_t bytes_written = 0;
  for (iovec const& i : iov) {
    bytes_written += riowrite(
      conn.socket,
      i.iov_base,
      i.iov_len,
      conn.offset,
      0);
  }

  ssize_t ret = rsend(conn.socket, &bytes_written, sizeof(bytes_written), 0);
  if (ret == -1) {
    LOG(WARNING) << "Error on rwritev: " << strerror(errno);
    return ret;
  }
  return bytes_written;
}

ssize_t lseb_read(BuConnectionId const& conn) {
  ssize_t bytes_read = 0;
  ssize_t ret = rrecv(
    conn.socket,
    &bytes_read,
    sizeof(bytes_read),
    MSG_WAITALL);
  if (ret == -1) {
    LOG(WARNING) << "Error on rrecv: " << strerror(errno);
    return ret;
  }
  return bytes_read;
}

}
