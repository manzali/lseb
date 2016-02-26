#ifndef TRANSPORT_ENDPOINTS_H
#define TRANSPORT_ENDPOINTS_H

#include <string>
#include <sstream>
#include <iostream>
#include <vector>

#include <cassert>

#include "common/configuration.h"

#include "launcher/LauncherHydra.hpp"

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

inline std::vector<Endpoint> get_endpoints(DAQ::LauncherHydra & launcher,std::string & port) {
  std::vector<Endpoint> endpoints;
  int nodes = launcher.getWorldSize();
  for (int i = 0 ; i < nodes ; i++)
    endpoints.emplace_back(launcher.get("ip",i),port);
  return endpoints;
}

}

#endif
