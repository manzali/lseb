#ifndef TRANSPORT_TRANSPORT_RSOCKETS_H
#define TRANSPORT_TRANSPORT_RSOCKETS_H

#include <vector>
#include <iostream>
#include <sys/poll.h>
#include <sys/uio.h>

namespace lseb {

struct RuConnectionId {
  int socket;
  off_t offset;
  RuConnectionId(int socket, off_t offset)
      :
        socket(socket),
        offset(offset) {
  }
};

struct BuConnectionId {
  int socket;
  void* buffer;
  size_t len;
  BuConnectionId(int socket, void* buffer, size_t len)
      :
        socket(socket),
        buffer(buffer),
        len(len){
  }
};

using BuSocket = int;

RuConnectionId lseb_connect(std::string const& hostname, long port);
BuSocket lseb_listen(std::string const& hostname, long port);
BuConnectionId lseb_accept(BuSocket const& socket, void* buffer, size_t len);
int lseb_poll(std::vector<pollfd>& poll_fds, int timeout_ms);
ssize_t lseb_write(RuConnectionId const& conn, std::vector<iovec> const& iov);
ssize_t lseb_read(BuConnectionId const& conn);

}

#endif
