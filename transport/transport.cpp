#include "transport/transport.h"

#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "common/log.h"

namespace lseb {

int lseb_connect(std::string const& hostname, long port) {
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    LOG(WARNING) << "Error on socket creation: " << strerror(errno);
    return -1;
  }
  sockaddr_in serv_addr;
  bzero((char*) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  hostent* server = gethostbyname(hostname.c_str());
  bcopy((char*) server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
  serv_addr.sin_port = htons(port);
  if (connect(sockfd, (sockaddr*) &serv_addr, sizeof(serv_addr)) != 0) {
    LOG(WARNING) << "Error on connect: " << strerror(errno);
    return -1;
  }
  LOG(DEBUG) << "Connected to host " << inet_ntoa(serv_addr.sin_addr)
             << " on port " << port;
  return sockfd;
}

int lseb_listen(std::string const& hostname, long port) {
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    LOG(WARNING) << "Error on socket creation: " << strerror(errno);
    return -1;
  }
  sockaddr_in serv_addr;
  bzero((char*) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  hostent* server = gethostbyname(hostname.c_str());
  bcopy((char*) server->h_addr, (char*) &serv_addr.sin_addr.s_addr, server->h_length);
  if (bind(sockfd, (sockaddr*) &serv_addr, sizeof(serv_addr))) {
    LOG(WARNING) << "Error on bind: " << strerror(errno);
    return -1;
  }
  listen(sockfd, 128);  // 128 seems to be the max number of waiting sockets in linux
  LOG(DEBUG) << "Host " << inet_ntoa(serv_addr.sin_addr)
             << " is listening on port " << port;
  return sockfd;
}

int lseb_accept(int sockfd) {
  sockaddr_in cli_addr;
  socklen_t clilen = sizeof(cli_addr);
  int newsockfd = accept(sockfd, (sockaddr*) &cli_addr, &clilen);
  if (newsockfd == -1) {
    LOG(WARNING) << "Error on accept: " << strerror(errno);
  } else {
    LOG(DEBUG) << "Host " << inet_ntoa(cli_addr.sin_addr)
               << " connected on port " << ntohs(cli_addr.sin_port);
  }
  return newsockfd;
}

int lseb_poll(std::vector<pollfd>& poll_fds, int timeout_ms) {
  int ret = poll(poll_fds.data(), poll_fds.size(), timeout_ms);
  if (ret == -1) {
    LOG(WARNING) << "Error on poll: " << strerror(errno);
  }
  //else {
  //  LOG(DEBUG) << "There are " << ret << " sockets ready to be read";
  //}
  return ret;
}

ssize_t lseb_write(int sockfd, std::vector<iovec> const& iov) {
  ssize_t ret = writev(sockfd, iov.data(), iov.size());
  if (ret == -1) {
    LOG(WARNING) << "Error on writev: " << strerror(errno);
  }
  //else {
  //  LOG(DEBUG) << "Write " << ret << " bytes on socket " << sockfd;
  //}
  return ret;
}

ssize_t lseb_read(int sockfd, void* buffer, size_t nbytes) {
  ssize_t ret = read(sockfd, buffer, nbytes);
  if (ret == -1) {
    LOG(WARNING) << "Error on read: " << strerror(errno);
  }
  //else {
  //  LOG(DEBUG) << "Read " << ret << " bytes on socket " << sockfd;
  //}
  return ret;
}

}
