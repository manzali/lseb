#ifndef TRANSPORT_TRANSPORT_RSOCKETS_H
#define TRANSPORT_TRANSPORT_RSOCKETS_H

#include <vector>
#include <iostream>
#include <sys/poll.h>
#include <sys/uio.h>

#define FIRST_BUFFER_LOCKED 0
#define SECOND_BUFFER_LOCKED 1
#define NO_LOCKS 2

namespace lseb {

struct RuConnectionId {
  int socket;
  uint8_t volatile poll;
  off_t avail_offset;
  off_t buffer_offset_1;
  off_t buffer_offset_2;
  size_t buffer_len;
  bool first_buffer;
  size_t buffer_written;
  RuConnectionId(int socket)
      :
        socket(socket),
        poll(NO_LOCKS),
        avail_offset(-1),
        buffer_offset_1(-1),
        buffer_offset_2(-1),
        buffer_len(0),
        first_buffer(true),
        buffer_written(0) {
  }
};

struct BuConnectionId {
  int socket;
  size_t volatile avail;
  void volatile* buffer;
  size_t buffer_len;
  bool first_half;
  off_t poll_offset;
  BuConnectionId(int socket, void* buffer, size_t buffer_len)
      :
        socket(socket),
        avail(0),
        buffer(buffer),
        buffer_len(buffer_len),
        first_half(true),
        poll_offset(-1) {
  }
};

using BuSocket = int;

RuConnectionId lseb_connect(std::string const& hostname, long port);
BuSocket lseb_listen(std::string const& hostname, long port);
BuConnectionId lseb_accept(BuSocket const& socket, void* buffer, size_t len);
bool lseb_sync(RuConnectionId& conn);
bool lseb_sync(BuConnectionId& conn);
bool lseb_poll(RuConnectionId& conn);
bool lseb_poll(BuConnectionId& conn);
ssize_t lseb_write(RuConnectionId& conn, std::vector<iovec>& iov);
std::vector<iovec> lseb_read(BuConnectionId& conn);
void lseb_release(BuConnectionId& conn);

}

#endif
