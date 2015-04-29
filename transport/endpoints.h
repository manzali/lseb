#ifndef TRANSPORT_ENDPOINTS_H
#define TRANSPORT_ENDPOINTS_H

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
      :
        m_hostname(hostname),
        m_port(port) {
    assert(m_port > 0 && "Invalid port number");
  }
  std::string hostname() const {
    return m_hostname;
  }
  int port() const {
    return m_port;
  }
  friend std::ostream& operator<<(std::ostream& os, Endpoint const& endpoint) {
    os << endpoint.hostname() << ":" << endpoint.port();
    return os;
  }
};

using Endpoints = std::vector<Endpoint>;

inline Endpoints get_endpoints(std::string const& str) {
  Endpoints endpoints;
  std::stringstream ss(str);
  std::string token;
  while (std::getline(ss, token, ':')) {
    std::string hostname = token;
    std::getline(ss, token, ' ');
    endpoints.emplace_back(std::move(hostname), std::move(std::stol(token)));
  }
  return endpoints;
}

}

#endif
