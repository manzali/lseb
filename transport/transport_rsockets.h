#ifndef TRANSPORT_TRANSPORT_RSOCKETS_H
#define TRANSPORT_TRANSPORT_RSOCKETS_H

#include <vector>
#include <iostream>
#include <sys/poll.h>
#include <sys/uio.h>

#define FREE 0
#define LOCKED 1

namespace lseb {

struct RuConnectionId {
  int socket;
  uint8_t volatile poll;
  off_t buffer_offset;
  off_t avail_offset;
  size_t buffer_len;
  size_t buffer_written;
  bool is_first;
  bool is_full;
  RuConnectionId(int socket)
      :
        socket(socket),
        poll(FREE),
        buffer_offset(-1),
        avail_offset(-1),
        buffer_len(0),
        buffer_written(0),
        is_first(true),
        is_full(false) {
  }
};

struct BuConnectionId {
  int socket;
  size_t volatile* avail;
  void volatile* buffer;
  size_t buffer_len;
  off_t poll_offset;
  bool is_first;
  bool is_done;
  BuConnectionId(int socket, void* buffer, size_t buffer_len)
      :
        socket(socket),
        avail(nullptr),
        buffer(buffer),
        buffer_len(buffer_len),
        poll_offset(-1),
        is_first(true),
        is_done(false) {
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
ssize_t lseb_write(RuConnectionId& conn, std::vector<iovec> const& iov);
std::vector<iovec> lseb_read(BuConnectionId& conn);
void lseb_release(BuConnectionId& conn);

}

#endif
