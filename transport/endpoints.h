#ifndef TRANSPORT_ENDPOINTS_H
#define TRANSPORT_ENDPOINTS_H

#include <string>
#include <sstream>
#include <iostream>
#include <vector>

#include <cassert>

#include "common/configuration.h"

namespace lseb {

class Endpoint {
  std::string m_hostname;
  std::string m_port;

 public:
  Endpoint(std::string const& hostname, std::string const& port)
      :
        m_hostname(hostname),
        m_port(port) {
  }
  std::string hostname() const {
    return m_hostname;
  }
  std::string port() const {
    return m_port;
  }
  friend std::ostream& operator<<(std::ostream& os, Endpoint const& endpoint) {
    os << endpoint.hostname() << ":" << endpoint.port();
    return os;
  }
};

inline std::vector<Endpoint> get_endpoints(Configuration const& configuration) {
  std::vector<Endpoint> endpoints;
  for (Configuration::const_iterator it = std::begin(configuration), e =
    std::end(configuration); it != e; ++it) {
    endpoints.emplace_back(
      it->second.get < std::string > ("HOST"),
      it->second.get < std::string > ("PORT"));
  }
  return endpoints;
}

}

#endif
