#include "transport/transport_verbs.h"

#include <algorithm>
#include <iterator>
#include <thread>
#include <chrono>

#include <cassert>
#include <cstring>

#include <infiniband/verbs.h>
#include <rdma/rdma_verbs.h>
#include <arpa/inet.h>

#define MAX_BACKLOG 128

namespace lseb {

namespace {
auto retry_wait = std::chrono::milliseconds(500);
}

RuConnectionId lseb_connect(
  std::string const& hostname,
  std::string const& port,
  int tokens) {

  RuConnectionId conn;
  conn.tokens = tokens;

  bool retry = false;

  do {
    retry = false;

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
    ret = rdma_create_ep(&conn.cm_id, res, NULL, &attr);
    if (ret) {
      throw std::runtime_error(
        "Error on rdma_create_ep: " + std::string(strerror(errno)));
    }

    ret = rdma_connect(conn.cm_id, NULL);
    if (ret) {
      rdma_destroy_ep(conn.cm_id);
      if (errno == ECONNREFUSED) {
        retry = true;
        std::this_thread::sleep_for(retry_wait);
      } else {
        throw std::runtime_error(
          "Error on rdma_connect: " + std::string(strerror(errno)));
      }
    }

    rdma_freeaddrinfo(res);

  } while (retry);

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

  int ret = rdma_create_ep(&socket.cm_id, res, NULL, &attr);
  rdma_freeaddrinfo(res);

  if (ret) {
    throw std::runtime_error(
      "Error on rdma_create_ep: " + std::string(strerror(errno)));
  }

  if (rdma_listen(socket.cm_id, MAX_BACKLOG)) {
    throw std::runtime_error(
      "Error on rdma_listen: " + std::string(strerror(errno)));
  }

  return socket;
}

BuConnectionId lseb_accept(BuSocket const& socket) {
  BuConnectionId conn;
  conn.wr_vect.resize(socket.tokens, nullptr);
  if (rdma_get_request(socket.cm_id, &conn.cm_id)) {
    throw std::runtime_error(
      "Error on rdma_get_request: " + std::string(strerror(errno)));
  }

  return conn;
}

std::string lseb_get_peer_hostname(BuConnectionId& conn) {
  sockaddr_in& addr = *((sockaddr_in*) rdma_get_peer_addr(conn.cm_id));
  return inet_ntoa(addr.sin_addr);
}

void lseb_register(RuConnectionId& conn, int id, void* buffer, size_t len) {
  conn.id = id;
  conn.mr = rdma_reg_msgs(conn.cm_id, buffer, len);
  if (!conn.mr) {
    throw std::runtime_error(
      "Error on rdma_reg_msgs: " + std::string(strerror(errno)));
  }

  // Send the id with inline flag
  ibv_sge sge;
  sge.addr = (uint64_t) (uintptr_t) &conn.id;
  sge.length = (uint32_t) sizeof(conn.id);
  ibv_send_wr wr;
  wr.next = nullptr;
  wr.sg_list = &sge;
  wr.num_sge = 1;
  wr.opcode = IBV_WR_SEND;
  wr.send_flags = IBV_SEND_INLINE;
  ibv_send_wr* bad_wr;
  int ret = ibv_post_send(conn.cm_id->qp, &wr, &bad_wr);
  if (ret) {
    throw std::runtime_error(
      "Error on ibv_post_send: " + std::string(strerror(ret)));
  }

  // Wait for completion
  ibv_wc wc;
  ret = 0;
  while (ret == 0) {
    // sleep ?
    ret = ibv_poll_cq(conn.cm_id->send_cq, 1, &wc);
    if (ret < 0) {
      throw std::runtime_error(
        "Error on ibv_poll_cq: " + std::string(strerror(ret)));
    }
  }
}

void lseb_register(BuConnectionId& conn, void* buffer, size_t len) {
  conn.mr = rdma_reg_msgs(conn.cm_id, buffer, len);
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

  if (rdma_accept(conn.cm_id, NULL)) {
    throw std::runtime_error(
      "Error on rdma_accept: " + std::string(strerror(errno)));
  }

  // Wait for completion
  ibv_wc wc;
  int ret = 0;
  while (ret == 0) {
    // sleep ?
    ret = ibv_poll_cq(conn.cm_id->recv_cq, 1, &wc);
    if (ret < 0) {
      throw std::runtime_error(
        "Error on ibv_poll_cq: " + std::string(strerror(ret)));
    }
  }

  // Set the id
  conn.id = *(static_cast<int*>(conn.wr_vect[wc.wr_id]));

  // Release the wr
  ibv_sge sge;
  sge.addr = (uint64_t) (uintptr_t) conn.wr_vect[wc.wr_id];
  sge.length = (uint32_t) conn.wr_len;
  sge.lkey = conn.mr ? conn.mr->lkey : 0;
  ibv_recv_wr wr;
  wr.wr_id = wc.wr_id;
  wr.next = nullptr;
  wr.sg_list = &sge;
  wr.num_sge = 1;
  ibv_recv_wr* bad_wr;
  ret = ibv_post_recv(conn.cm_id->qp, &wr, &bad_wr);
  if (ret) {
    throw std::runtime_error(
      "Error on ibv_post_recv: " + std::string(strerror(ret)));
  }
}

int lseb_avail(RuConnectionId& conn) {
  if (conn.wr_count) {
    std::vector<ibv_wc> wcs(conn.wr_count);
    int ret = ibv_poll_cq(conn.cm_id->send_cq, wcs.size(), &wcs.front());
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
    int ret = ibv_post_send(conn.cm_id->qp, &(wrs.front().first), nullptr);
    if (ret) {
      throw std::runtime_error(
        "Error on ibv_post_send: " + std::string(strerror(ret)));
    }
  }

  return bytes_sent;
}

std::vector<iovec> lseb_read(BuConnectionId& conn) {
  std::vector<ibv_wc> wcs(conn.wr_count);
  int ret = ibv_poll_cq(conn.cm_id->recv_cq, wcs.size(), &wcs.front());
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

  if (!wrs.empty()) {
    int ret = ibv_post_recv(conn.cm_id->qp, &(wrs.front().first), nullptr);
    if (ret) {
      throw std::runtime_error(
        "Error on ibv_post_recv: " + std::string(strerror(ret)));
    }
  }
}

}
