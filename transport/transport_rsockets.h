#ifndef TRANSPORT_TRANSPORT_RSOCKETS_H
#define TRANSPORT_TRANSPORT_RSOCKETS_H

#include <vector>
#include <iostream>
#include <sys/poll.h>
#include <sys/uio.h>

#define READY_TO_READ 1
#define READY_TO_WRITE 0

namespace lseb {

struct RuConnectionId {
  int socket;
  uint8_t volatile poll_byte;
  off_t offset;
  RuConnectionId(int socket)
      :
        socket(socket),
        poll_byte(READY_TO_WRITE),
        offset(-1) {
  }
};

struct BuConnectionId {
  int socket;
  off_t offset;
  void volatile* buffer;
  size_t len;
  uint64_t event_id;
  BuConnectionId(int socket, void* buffer, size_t len)
      :
        socket(socket),
        offset(-1),
        buffer(buffer),
        len(len),
        event_id(0) {
  }
};

using BuSocket = int;

RuConnectionId lseb_connect(std::string const& hostname, long port);
BuSocket lseb_listen(std::string const& hostname, long port);
BuConnectionId lseb_accept(BuSocket const& socket, void* buffer, size_t len);
bool lseb_register(RuConnectionId& conn);
bool lseb_register(BuConnectionId& conn);
bool lseb_poll(RuConnectionId& conn);
bool lseb_poll(BuConnectionId& conn);
ssize_t lseb_write(RuConnectionId& conn, std::vector<iovec> const& iov);
ssize_t lseb_read(BuConnectionId& conn, size_t events_in_multievent);

}

#endif
