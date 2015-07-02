#include "transport/transport_verbs.h"

#include <rdma/rdma_verbs.h>

namespace lseb {

RuConnectionId lseb_connect(
  std::string const& hostname,
  std::string const& port) {

  rdma_addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_port_space = RDMA_PS_TCP;

  rdma_addrinfo* res;
  int ret = rdma_getaddrinfo(
    (char*) hostname.c_str(),
    (char*) port.c_str(),
    &hints,
    &res);
  if (ret) {
    rdma_freeaddrinfo(res);
    throw std::runtime_error(
      "Error on rdma_getaddrinfo: " + std::string(strerror(errno)));
  }

  ibv_qp_init_attr attr;
  memset(&attr, 0, sizeof(attr));
  attr.cap.max_send_wr = 1024;  // maybe it can be passed as argument?
  attr.cap.max_send_sge = 2;  // in case of wrap of multievent (to check)
  attr.cap.max_recv_wr = 1;
  attr.cap.max_recv_sge = 1;
  attr.sq_sig_all = 1;

  RuConnectionId conn;

  ret = rdma_create_ep(&conn.id, res, NULL, &attr);
  if (ret) {
    throw std::runtime_error(
      "Error on rdma_create_ep: " + std::string(strerror(errno)));
  }

  if (rdma_connect(conn.id, NULL)) {
    rdma_dereg_mr(conn.mr);
    rdma_destroy_ep(conn.id);
    throw std::runtime_error(
      "Error on rdma_connect: " + std::string(strerror(errno)));
  }

  return conn;
}

BuSocket lseb_listen(std::string const& hostname, std::string const& port) {

  rdma_addrinfo hints;
  memset(&hints, 0, sizeof hints);
  hints.ai_flags = RAI_PASSIVE;
  hints.ai_port_space = RDMA_PS_TCP;

  rdma_addrinfo* res;
  if (rdma_getaddrinfo(
    (char*) hostname.c_str(),
    (char*) port.c_str(),
    &hints,
    &res)) {
    rdma_freeaddrinfo(res);
    throw std::runtime_error(
      "Error on rdma_getaddrinfo: " + std::string(strerror(errno)));
  }

  ibv_qp_init_attr attr;
  memset(&attr, 0, sizeof(attr));
  attr.cap.max_send_wr = 1;
  attr.cap.max_recv_wr = 1024;  // maybe it can be passed as argument?
  attr.cap.max_send_sge = attr.cap.max_recv_sge = 1;
  attr.sq_sig_all = 1;
  BuSocket socket;

  int ret = rdma_create_ep(&socket.id, res, NULL, &attr);
  rdma_freeaddrinfo(res);

  if (ret) {
    throw std::runtime_error(
      "Error on rdma_create_ep: " + std::string(strerror(errno)));
  }

  if (rdma_listen(socket.id, 128)) {
    rdma_destroy_ep(socket.id);
    throw std::runtime_error(
      "Error on rdma_listen: " + std::string(strerror(errno)));
  }

  return socket;
}

BuConnectionId lseb_accept(BuSocket const& socket, void* buffer, size_t len) {
  BuConnectionId conn;
  if (rdma_get_request(socket.id, &conn.id)) {
    rdma_destroy_ep(socket.id);
    throw std::runtime_error(
      "Error on rdma_get_request: " + std::string(strerror(errno)));
  }
  return conn;
}

}
