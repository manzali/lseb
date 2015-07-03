#ifndef TRANSPORT_TRANSPORT_VERBS_H
#define TRANSPORT_TRANSPORT_VERBS_H

#include <rdma/rdma_cma.h>

#include "common/utility.h"

namespace lseb {

struct RuConnectionId {
  rdma_cm_id* id;
  ibv_mr* mr;
  int tokens;
  int wr_count;
};

struct BuConnectionId {
  rdma_cm_id* id;
  ibv_mr* mr;
  int tokens;
  size_t wr_len;
  std::map<int, void*> wr_map;
};

struct BuSocket {
  rdma_cm_id* id;
  int tokens;
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

void lseb_register(RuConnectionId& conn, void* buffer, size_t len);
void lseb_register(BuConnectionId& conn, void* buffer, size_t len);

bool lseb_poll(RuConnectionId& conn);
size_t lseb_write(RuConnectionId& conn, DataIov const& iov);

std::vector<iovec> lseb_read(BuConnectionId& conn);
void lseb_release(BuConnectionId& conn, std::vector<iovec> const& iov);

}

#endif
