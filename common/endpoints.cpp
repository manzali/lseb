#include "common/endpoints.h"

namespace lseb {

std::ostream& operator<<(std::ostream& os, Endpoint const& endpoint){
  os << endpoint.hostname() << ":" << endpoint.port();
  return os;
}

Endpoints get_endpoints(std::string const& str) {
  Endpoints endpoints;
  std::stringstream ss(str);
  std::string token;
  while (std::getline(ss, token, ':')) {
    std::string hostname = token;
    std::getline(ss, token, ' ');
    endpoints.push_back(Endpoint(hostname, std::stol(token)));
  }
  return endpoints;
}

}
