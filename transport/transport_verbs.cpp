#include "transport/transport_verbs.h"

#include <cassert>

#include <infiniband/verbs.h>
#include <rdma/rdma_verbs.h>

#define MAX_BACKLOG 128

int find_lowest_avail_key(std::map<int, void*> const& wr_map) {
  int key = 0;
  for (auto it = std::begin(wr_map); it != std::end(wr_map) && it->first == key;
      ++it) {
    ++key;
  }
  return key;
}

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
    throw std::runtime_error(
      "Error on rdma_listen: " + std::string(strerror(errno)));
  }

  return socket;
}

BuConnectionId lseb_accept(BuSocket const& socket) {
  BuConnectionId conn;
  conn.tokens = socket.tokens;
  if (rdma_get_request(socket.id, &conn.id)) {
    throw std::runtime_error(
      "Error on rdma_get_request: " + std::string(strerror(errno)));
  }
  return conn;
}

void lseb_register(RuConnectionId& conn, void* buffer, size_t len) {
  conn.mr = rdma_reg_msgs(conn.id, buffer, len);
  if (!conn.mr) {
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
  conn.wr_len = len / conn.tokens;
  assert(conn.wr_len != 0 && "Too much tokens");
  for (int i = 0; i < conn.tokens; ++i) {
    auto it = conn.wr_map.find(i);
    assert(it == std::end(conn.wr_map));
    it = conn.wr_map.insert(it, std::make_pair(i, buffer + conn.wr_len * i));
    rdma_post_recv(
      conn.id,
      (void*) it->first,
      it->second,
      conn.wr_len,
      conn.mr);
  }

  if (rdma_accept(conn.id, NULL)) {
    throw std::runtime_error(
      "Error on rdma_accept: " + std::string(strerror(errno)));
  }
}

bool lseb_poll(RuConnectionId& conn) {
  std::vector<ibv_wc> wcs(conn.tokens);
  int ret = ibv_poll_cq(conn.id->send_cq, wcs.size(), &wcs.front());
  if (ret < 0) {
    throw std::runtime_error(
      "Error on ibv_poll_cq: " + std::string(strerror(ret)));
  }
  conn.wr_count -= ret;
  if (conn.wr_count != conn.tokens) {
    return true;
  }
  return false;
}

size_t lseb_write(RuConnectionId& conn, DataIov const& iov) {

  if (conn.wr_count == conn.tokens) {
    while (!lseb_poll(conn)) {
      ;
    }
  }

  // create scatter gather elements (max 2 elements!)
  assert(iov.size <= 2 && "Size of DataIov can be only 1 or 2");

  size_t bytes_sent = 0;
  std::vector<ibv_sge> sges;
  for (auto& i : iov) {
    ibv_sge sge;
    sge.addr = (uint64_t) (uintptr_t) i.iov_base;
    sge.length = (uint32_t) i.iov_len;
    sge.lkey = conn.mr ? conn.mr->lkey : 0;
    sges.push_back(sge);
    bytes_sent += i.iov_len;
  }

  if (rdma_post_sendv(conn.id, nullptr, &sges.front(), sges.size(), 0)) {
    throw std::runtime_error(
      "Error on rdma_post_sendv: " + std::string(strerror(errno)));
  }

  ++conn.wr_count;
  return bytes_sent;
}

std::vector<iovec> lseb_read(BuConnectionId& conn) {

  std::vector<ibv_wc> wcs(conn.tokens);
  int ret = ibv_poll_cq(conn.id->recv_cq, wcs.size(), &wcs.front());
  if (ret < 0) {
    throw std::runtime_error(
      "Error on ibv_poll_cq: " + std::string(strerror(ret)));
  }

  std::vector<iovec> iov;
  for (int i = 0; i < ret; ++i) {
    auto it = conn.wr_map.find(wcs[i].wr_id);
    assert(it != std::end(wr_map));
    iov.push_back( { it->second.iov_base, wcs[i].byte_len });
    conn.wr_map.erase(it);
  }

  return iov;
}

void lseb_release(BuConnectionId& conn, std::vector<iovec> const& iov) {
  for (auto& i : iov) {
    int key = find_lowest_avail_key(conn.wr_map);
    auto it = conn.wr_map.find(key);
    assert(it == std::end(conn.wr_map));
    it = conn.wr_map.insert(it, std::make_pair(key, i.iov_base));
    rdma_post_recv(
      conn.id,
      (void*) it->first,
      it->second.iov_base,
      conn.wr_len,
      conn.mr);
  }
}

}
