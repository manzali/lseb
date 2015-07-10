#ifndef RU_SENDER_H
#define RU_SENDER_H

#include <map>
#include <vector>
#include <chrono>

#include "common/dataformat.h"

#include "transport/transport.h"

namespace lseb {

class Sender {
  std::vector<RuConnectionId> m_connection_ids;

 public:
  Sender(std::vector<RuConnectionId> const& connection_ids);
  size_t send(
    std::map<int, std::vector<DataIov> > const& iov_map,
    std::chrono::milliseconds const& ms_timeout);
};

}

#endif
