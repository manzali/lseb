#ifndef BU_RECEIVER_H
#define BU_RECEIVER_H

#include <vector>

#include "common/dataformat.h"

#include "transport/transport.h"

namespace lseb {

class Receiver {
  std::vector<BuConnectionId> m_connection_ids;

 private:
  bool checkData(std::vector<iovec> total_iov);

 public:
  Receiver(std::vector<BuConnectionId> const& connection_ids);
  size_t receive();
  size_t receiveAndForget();
};

}

#endif
