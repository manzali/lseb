#ifndef TRANSPORT_VERBS_CONNECTOR_H
#define TRANSPORT_VERBS_CONNECTOR_H

#include <memory>

#include "transport/libfabric/socket.h"

namespace lseb {

class Connector {
public:

  Connector(int credits);
  Connector(Connector const& other) = delete;
  Connector& operator=(Connector const&) = delete;
  Connector(Connector&& other) = delete;
  Connector& operator=(Connector&&) = delete;
  ~Connector() = default;

  std::unique_ptr<Socket> connect(
      std::string const& hostname,
      std::string const& port);
private:

  uint32_t m_credits;

};

}

#endif
