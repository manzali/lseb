#ifndef TRANSPORT_TRANSPORT_TCP_H
#define TRANSPORT_TRANSPORT_TCP_H

#include <vector>
#include <iostream>
#include <sys/poll.h>
#include <sys/uio.h>

namespace lseb {

struct RuConnectionId {
  int socket;
  RuConnectionId(int socket)
      :
        socket(socket) {
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
        len(len) {
  }
};

using BuSocket = int;

RuConnectionId lseb_connect(std::string const& hostname, long port);
BuSocket lseb_listen(std::string const& hostname, long port);
BuConnectionId lseb_accept(BuSocket const& socket, void* buffer, size_t len);
bool lseb_register(RuConnectionId const& conn);
bool lseb_register(BuConnectionId const& conn);
bool lseb_poll(RuConnectionId const& conn);
bool lseb_poll(BuConnectionId const& conn);
ssize_t lseb_write(RuConnectionId const& conn, std::vector<iovec>& iov);
ssize_t lseb_read(BuConnectionId& conn);

}

#endif
