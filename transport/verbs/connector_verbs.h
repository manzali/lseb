#ifndef TRANSPORT_VERBS_CONNECTOR_VERBS_H
#define TRANSPORT_VERBS_CONNECTOR_VERBS_H

#include <type_traits>

#include <infiniband/verbs.h>
#include <rdma/rdma_verbs.h>

#include "transport/verbs/socket_verbs.h"

namespace lseb {

template<typename T>
class Connector {

  int m_credits;

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
  Connector(int credits)
      :
        m_credits(credits) {
  }

  ~Connector() {
    // To be filled
  }

  std::unique_ptr<T> connect(std::string const& hostname, std::string const& port) {
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
    std::unique_ptr<T> socket(new T(cm_id, m_credits));
    return socket;
  }
};

}

#endif
