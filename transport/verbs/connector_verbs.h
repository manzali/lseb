#ifndef TRANSPORT_VERBS_CONNECTOR_VERBS_H
#define TRANSPORT_VERBS_CONNECTOR_VERBS_H

#include <type_traits>

#include <infiniband/verbs.h>
#include <rdma/rdma_verbs.h>

#include "common/utility.h"

#include "transport/verbs/socket_verbs.h"

namespace lseb {

template<typename T>
class Connector {

  void* m_buffer;
  size_t m_size;
  int m_credits;
  ibv_mr* m_mr;

 private:

  rdma_addrinfo* create_addr_info(
    std::string const& hostname,
    std::string const& port) {
    rdma_addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_port_space = RDMA_PS_TCP;
    rdma_addrinfo* res;
    int ret = rdma_getaddrinfo(
      const_cast<char*>(hostname.c_str()),
      const_cast<char*>(port.c_str()),
      &hints,
      &res);
    if (ret) {
      throw std::runtime_error(
        "Error on rdma_getaddrinfo: " + std::string(strerror(errno)));
    }
    return res;
  }

  void destroy_addr_info(rdma_addrinfo* res) {
    rdma_freeaddrinfo(res);
  }

 public:
  Connector(void* buffer, size_t size, int credits)
      :
        m_buffer(buffer),
        m_size(size),
        m_credits(credits),
        m_mr(nullptr) {
  }

  T connect(std::string const& hostname, std::string const& port) {
    auto res = create_addr_info(hostname, port);
    auto attr = T::create_qp_attr(m_credits);
    rdma_cm_id* cm_id;

    int ret = rdma_create_ep(&cm_id, res, NULL, &attr);
    destroy_addr_info(res);
    if (ret) {
      throw std::runtime_error(
        "Error on rdma_create_ep: " + std::string(strerror(errno)));
    }

    if (rdma_connect(cm_id, NULL)) {
      rdma_destroy_ep(cm_id);
      if (errno == ECONNREFUSED) {
        throw std::runtime_error(
          "Error on rdma_connect: " + std::string(strerror(errno)));
      } else {
        throw std::runtime_error(
          "Error on rdma_connect: " + std::string(strerror(errno)));
      }
    }

    if (!m_mr) {
      m_mr = rdma_reg_msgs(cm_id, m_buffer, m_size);
      if (!m_mr) {
        throw std::runtime_error(
          "Error on rdma_reg_msgs: " + std::string(strerror(errno)));
      }
    }

    return T(cm_id, m_mr, m_credits);
  }
};

}

#endif
