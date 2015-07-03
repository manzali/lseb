#ifndef TRANSPORT_TRANSPORT_VERBS_H
#define TRANSPORT_TRANSPORT_VERBS_H

#include <rdma/rdma_cma.h>

namespace lseb {

struct RuConnectionId {
  rdma_cm_id* id;
  ibv_mr* mr;
  int tokens;
  std::map<int, iovec> wr_map;
};

struct BuConnectionId {
  rdma_cm_id* id;
  ibv_mr* mr;
  int tokens;
  std::map<int, iovec> wr_map;
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
bool lseb_poll(BuConnectionId& conn);

size_t lseb_write(RuConnectionId& conn, std::vector<iovec> const& iov);
size_t lseb_read();

}

#endif
