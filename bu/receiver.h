#ifndef BU_RECEIVER_H
#define BU_RECEIVER_H

#include <vector>

#include "common/dataformat.h"

#include "transport/transport.h"

namespace lseb {

class Receiver {
  std::vector<BuConnectionId> m_connection_ids;

 public:
  Receiver(std::vector<BuConnectionId> const& connection_ids);
  size_t receive();
  size_t receive_and_forget();
};

}

#endif
