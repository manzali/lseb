#include "transport/transport.h"

#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>

#include "common/log.h"

namespace lseb {

static int counter = 0;

int lseb_connect(std::string hostname, long port) {
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  hostent* server = gethostbyname(hostname.c_str());
  sockaddr_in serv_addr;
  bzero((char*) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char*) server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
  serv_addr.sin_port = htons(port);
  if (connect(sockfd, (sockaddr*) &serv_addr, sizeof(serv_addr)) != 0) {
    LOG(WARNING) << "Error on connect: " << strerror(errno);
    return -1;
  }
  return sockfd;
}

int lseb_listen(std::string hostname, long port) {
  return counter++;  // returns the socket fd of the server
}

int lseb_accept(int conn_id) {
  return counter++;  // return the socket fd for communication with client
}

}
