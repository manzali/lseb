#include "transport/transport_verbs.h"

#include <cassert>

#include <rdma/rdma_verbs.h>

#define MAX_BACKLOG 128

namespace lseb {

RuConnectionId lseb_connect(
  std::string const& hostname,
  std::string const& port,
  int tokens) {

  assert(tokens > 1);

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
  attr.cap.max_send_wr = tokens;  // maybe it can be passed as argument?
  attr.cap.max_send_sge = 2;  // in case of wrap of multievent (to check)
  attr.cap.max_recv_wr = 1;
  attr.cap.max_recv_sge = 1;
  attr.sq_sig_all = 1;

  RuConnectionId conn;
  conn.tokens = tokens;

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

BuSocket lseb_listen(
  std::string const& hostname,
  std::string const& port,
  int tokens) {

  assert(tokens > 1);

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
  attr.cap.max_recv_wr = tokens;  // maybe it can be passed as argument?
  attr.cap.max_send_sge = attr.cap.max_recv_sge = 1;
  attr.sq_sig_all = 1;

  BuSocket socket;
  socket.tokens = tokens;

  int ret = rdma_create_ep(&socket.id, res, NULL, &attr);
  rdma_freeaddrinfo(res);

  if (ret) {
    throw std::runtime_error(
      "Error on rdma_create_ep: " + std::string(strerror(errno)));
  }

  if (rdma_listen(socket.id, MAX_BACKLOG)) {
    rdma_destroy_ep(socket.id);
    throw std::runtime_error(
      "Error on rdma_listen: " + std::string(strerror(errno)));
  }

  return socket;
}

BuConnectionId lseb_accept(BuSocket const& socket) {
  BuConnectionId conn;
  conn.tokens = socket.tokens;
  if (rdma_get_request(socket.id, &conn.id)) {
    rdma_destroy_ep(socket.id);
    throw std::runtime_error(
      "Error on rdma_get_request: " + std::string(strerror(errno)));
  }
  return conn;
}

void lseb_register(RuConnectionId& conn, void* buffer, size_t len) {
  conn.mr = rdma_reg_msgs(conn.id, buffer, len);
  if (!conn.mr) {
    rdma_destroy_ep(conn.id);
    throw std::runtime_error(
      "Error on rdma_reg_msgs: " + std::string(strerror(errno)));
  }
}

void lseb_register(BuConnectionId& conn, void* buffer, size_t len) {
  conn.mr = rdma_reg_msgs(conn.id, buffer, len);
  if (!conn.mr) {
    rdma_destroy_ep(conn.id);
    throw std::runtime_error(
      "Error on rdma_reg_msgs: " + std::string(strerror(errno)));
  }

  assert(
    len % conn.tokens == 0 && "length of buffer not divisible by number of wr");
  size_t const chunk = len / conn.tokens;
  assert(chunk != 0 && "Too much tokens");
  for (int i = 0; i < conn.tokens; ++i) {
    auto it = conn.wr_map.find(i);
    assert(it == std::end(conn.wr_map));
    iovec iov = { buffer + chunk * i, chunk };
    it = conn.wr_map.insert(it, std::make_pair(i, iov));
    rdma_post_recv(conn.id, (void*) it->first,  // this is the wr_id
      it->second.iov_base,
      it->second.iov_len,
      conn.mr);
  }

  if (rdma_accept(conn.id, NULL)) {
    rdma_dereg_mr(conn.mr);
    rdma_destroy_ep(conn.id);
    throw std::runtime_error(
      "Error on rdma_accept: " + std::string(strerror(errno)));
  }
}

}
