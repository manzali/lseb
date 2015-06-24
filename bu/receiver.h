#ifndef BU_RECEIVER_H
#define BU_RECEIVER_H

#include <vector>

#include "transport/transport.h"

namespace lseb {

class Receiver {
  std::vector<BuConnectionId> m_connection_ids;

 public:
  Receiver(std::vector<BuConnectionId> const& connection_ids);
  std::vector<iovec> receive(std::chrono::milliseconds ms_timeout);
  void release();
};

}

#endif
