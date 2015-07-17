#include "transport/transport_verbs.h"

#include <algorithm>
#include <iterator>
#include <thread>
#include <chrono>

#include <cassert>
#include <cstring>

#include <infiniband/verbs.h>
#include <rdma/rdma_verbs.h>

#define MAX_BACKLOG 128

namespace lseb {

RuConnectionId lseb_connect(
  std::string const& hostname,
  std::string const& port,
  int tokens) {

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
  attr.cap.max_send_wr = tokens;
  attr.cap.max_send_sge = 2;
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

  do {
    ret = rdma_connect(conn.id, NULL);
    // 111 -> Connection refused
    if (errno != 111) {
      throw std::runtime_error(
        "Error on rdma_connect: " + std::string(strerror(errno)));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  } while (ret);

  return conn;
}

BuSocket lseb_listen(
  std::string const& hostname,
  std::string const& port,
  int tokens) {

  assert(tokens > 0);

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
  attr.cap.max_recv_wr = tokens;
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
  conn.wr_vect.resize(socket.tokens, nullptr);
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
    throw std::runtime_error(
      "Error on rdma_reg_msgs: " + std::string(strerror(errno)));
  }
  int tokens = conn.wr_vect.size();
  assert(len % tokens == 0 && "length of buffer not divisible by number of wr");
  conn.wr_len = len / tokens;
  assert(conn.wr_len != 0 && "Too much tokens");

  std::vector<iovec> wrs(tokens);
  for (int i = 0; i < tokens; ++i) {
    wrs[i].iov_base = buffer + conn.wr_len * i;
  }
  lseb_release(conn, wrs);

  if (rdma_accept(conn.id, NULL)) {
    throw std::runtime_error(
      "Error on rdma_accept: " + std::string(strerror(errno)));
  }
}

int lseb_avail(RuConnectionId& conn) {
  if (conn.wr_count) {
    std::vector<ibv_wc> wcs(conn.wr_count);
    int ret = ibv_poll_cq(conn.id->send_cq, wcs.size(), &wcs.front());
    if (ret < 0) {
      throw std::runtime_error(
        "Error on ibv_poll_cq: " + std::string(strerror(ret)));
    }
    conn.wr_count -= ret;
  }
  return conn.tokens - conn.wr_count;
}

bool lseb_poll(BuConnectionId& conn) {
  return conn.wr_count != 0;
}

size_t lseb_write(
  RuConnectionId& conn,
  std::vector<DataIov> const& data_iovecs) {

  assert(
    conn.tokens - conn.wr_count >= data_iovecs.size() && "Too much data to send");

  size_t bytes_sent = 0;

  ibv_send_wr* bad;
  std::vector<std::pair<ibv_send_wr, std::vector<ibv_sge> > > wrs(
    data_iovecs.size());

  for (int i = 0; i < wrs.size(); ++i) {

    // Get references
    ibv_send_wr& wr = wrs[i].first;
    std::vector<ibv_sge>& sges = wrs[i].second;
    DataIov const& iov = data_iovecs[i];

    for (auto const& i : iov) {
      ibv_sge sge;
      sge.addr = (uint64_t) (uintptr_t) i.iov_base;
      sge.length = (uint32_t) i.iov_len;
      sge.lkey = conn.mr ? conn.mr->lkey : 0;
      sges.push_back(sge);
      bytes_sent += i.iov_len;
    }

    wr.wr_id = (uintptr_t) nullptr;
    wr.next = (i + 1 == wrs.size()) ? nullptr : &(wrs[i + 1].first);
    wr.sg_list = &sges.front();
    wr.num_sge = sges.size();
    wr.opcode = IBV_WR_SEND;
    wr.send_flags = 0;

    ++conn.wr_count;
  }

  if (!wrs.empty()) {
    int ret = ibv_post_send(conn.id->qp, &(wrs.front().first), &bad);
    if (ret) {
      throw std::runtime_error(
        "Error on ibv_post_send: " + std::string(strerror(ret)));
    }
  }

  return bytes_sent;
}

std::vector<iovec> lseb_read(BuConnectionId& conn) {

  std::vector<ibv_wc> wcs(conn.wr_count);
  int ret = ibv_poll_cq(conn.id->recv_cq, wcs.size(), &wcs.front());
  if (ret < 0) {
    throw std::runtime_error(
      "Error on ibv_poll_cq: " + std::string(strerror(ret)));
  }

  std::vector<iovec> iov;
  for (int i = 0; i < ret; ++i) {
    assert(conn.wr_vect[wcs[i].wr_id] != nullptr);
    iov.push_back( { conn.wr_vect[wcs[i].wr_id], wcs[i].byte_len });
    conn.wr_vect[wcs[i].wr_id] = nullptr;
    --conn.wr_count;
  }

  return iov;
}

void lseb_release(BuConnectionId& conn, std::vector<iovec> const& credits) {

  std::vector<std::pair<ibv_recv_wr, ibv_sge> > wrs(credits.size());

  for (int i = 0; i < wrs.size(); ++i) {

    // Get references
    ibv_recv_wr& wr = wrs[i].first;
    ibv_sge& sge = wrs[i].second;

    // ...
    auto it = std::find_if(
      std::begin(conn.wr_vect),
      std::end(conn.wr_vect),
      [](void* p) {return p == nullptr;});
    assert(it != std::end(conn.wr_vect));
    *it = credits[i].iov_base;
    ++conn.wr_count;

    // ...
    sge.addr = (uint64_t) (uintptr_t) credits[i].iov_base;
    sge.length = (uint32_t) conn.wr_len;
    sge.lkey = conn.mr ? conn.mr->lkey : 0;

    // ...
    wr.wr_id = (uintptr_t) std::distance(std::begin(conn.wr_vect), it);
    wr.next = (i + 1 == wrs.size()) ? nullptr : &(wrs[i + 1].first);
    wr.sg_list = &(wrs[i].second);
    wr.num_sge = 1;
  }

  ibv_recv_wr* bad;
  if (!wrs.empty()) {
    int ret = ibv_post_recv(conn.id->qp, &(wrs.front().first), &bad);
    if (ret) {
      throw std::runtime_error(
        "Error on ibv_post_recv: " + std::string(strerror(ret)));
    }
  }
}

}
