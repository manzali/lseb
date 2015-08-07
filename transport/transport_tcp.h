#ifndef TRANSPORT_TRANSPORT_TCP_H
#define TRANSPORT_TRANSPORT_TCP_H

#include <vector>
#include <iostream>
#include <sys/poll.h>
#include <sys/uio.h>

#include "common/utility.h"

namespace lseb {

struct RuConnectionId {
  int id;
  int socket;
  RuConnectionId(int socket)
      :
        id(-1),
        socket(socket) {
  }
};

struct BuConnectionId {
  int id;
  int socket;
  void* buffer;
  size_t len;
  BuConnectionId(int socket)
      :
        id(-1),
        socket(socket) {
  }
};

struct BuSocket {
  int socket;
};

RuConnectionId lseb_connect(
  std::string const& hostname,
  std::string const& port,
  int tokens);

BuSocket lseb_listen(
  std::string const& hostname,
  std::string const& port,
  int tokens);

BuConnectionId lseb_accept(BuSocket const& socket);

std::string lseb_get_peer_hostname(RuConnectionId const& conn);
std::string lseb_get_peer_hostname(BuConnectionId const& conn);

void lseb_register(RuConnectionId& conn, int id, void* buffer, size_t len);
void lseb_register(BuConnectionId& conn, void* buffer, size_t len);

int lseb_avail(RuConnectionId const& conn);
bool lseb_poll(BuConnectionId const& conn);

ssize_t lseb_write(
  RuConnectionId const& conn,
  std::vector<DataIov> const& data_iovecs);
std::vector<iovec> lseb_read(BuConnectionId& conn);

void lseb_release(BuConnectionId& conn, std::vector<iovec> const& credits);

}

#endif
