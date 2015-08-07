#ifndef TRANSPORT_TRANSPORT_VERBS_H
#define TRANSPORT_TRANSPORT_VERBS_H

#include <map>

#include <rdma/rdma_cma.h>

#include "common/utility.h"

namespace lseb {

struct RuConnectionId {
  rdma_cm_id* cm_id;
  ibv_mr* mr;
  int tokens;
  int wr_count;
  RuConnectionId()
      :
        wr_count(0) {
  }
};

struct BuConnectionId {
  rdma_cm_id* cm_id;
  ibv_mr* mr;
  size_t wr_len;
  std::vector<void*> wr_vect;
  int wr_count;
  BuConnectionId()
      :
        wr_count(0) {
  }
};

struct BuSocket {
  rdma_cm_id* cm_id;
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
std::string lseb_get_peer_hostname(BuConnectionId& conn);

void lseb_register(RuConnectionId& conn, void* buffer, size_t len);
void lseb_register(BuConnectionId& conn, void* buffer, size_t len);

int lseb_avail(RuConnectionId& conn);
bool lseb_poll(BuConnectionId& conn);

size_t lseb_write(
  RuConnectionId& conn,
  std::vector<DataIov> const& data_iovecs);
std::vector<iovec> lseb_read(BuConnectionId& conn);

void lseb_release(BuConnectionId& conn, std::vector<iovec> const& credits);

}

#endif
