#ifndef COMMON_ENDPOINTS_H
#define COMMON_ENDPOINTS_H

#include <string>
#include <sstream>
#include <iostream>
#include <vector>

#include <cassert>

namespace lseb {

class Endpoint {
  std::string m_hostname;
  int m_port;

 public:
  Endpoint(std::string const& hostname, int port)
      : m_hostname(hostname),
        m_port(port) {
    assert(m_port > 0 && "Invalid port number");
  }
  std::string hostname() const {
    return m_hostname;
  }
  int port() const {
    return m_port;
  }
};

using Endpoints = std::vector<Endpoint>;

std::ostream& operator<<(std::ostream& os, Endpoint const& endpoint);

Endpoints get_endpoints(std::string const& str);

}

#endif
