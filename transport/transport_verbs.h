#ifndef TRANSPORT_TRANSPORT_VERBS_H
#define TRANSPORT_TRANSPORT_VERBS_H

#include <rdma/rdma_cma.h>

namespace lseb {

struct RuConnectionId {
  rdma_cm_id* id;
  ibv_mr* mr;
};

struct BuConnectionId {
  rdma_cm_id* id;
  ibv_mr* mr;
};

struct BuSocket {
  rdma_cm_id* id;
};

RuConnectionId lseb_connect(
  std::string const& hostname,
  std::string const& port);
BuSocket lseb_listen(std::string const& hostname, std::string const& port);
BuConnectionId lseb_accept(BuSocket const& socket, void* buffer, size_t len);

}

#endif
