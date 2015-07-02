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
  size_t avail;
  BuConnectionId(int socket, void* buffer, size_t len)
      :
        socket(socket),
        buffer(buffer),
        len(len),
        avail(0) {
  }
};

using BuSocket = int;

RuConnectionId lseb_connect(
  std::string const& hostname,
  std::string const& port);
BuSocket lseb_listen(std::string const& hostname, std::string const& port);
BuConnectionId lseb_accept(BuSocket const& socket, void* buffer, size_t len);
bool lseb_sync(RuConnectionId const& conn);
bool lseb_sync(BuConnectionId const& conn);
bool lseb_poll(RuConnectionId const& conn);
bool lseb_poll(BuConnectionId const& conn);
ssize_t lseb_write(RuConnectionId const& conn, std::vector<iovec> const& iov);
std::vector<iovec> lseb_read(BuConnectionId& conn);
void lseb_release(BuConnectionId& conn);

}

#endif
