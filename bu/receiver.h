#ifndef BU_RECEIVER_H
#define BU_RECEIVER_H

#include <vector>
#include <map>

#include "transport/transport.h"

namespace lseb {

class Receiver {
  std::vector<BuConnectionId> m_connection_ids;

 public:
  Receiver(std::vector<BuConnectionId> const& connection_ids);
  std::vector<iovec> receive(int conn);
  std::map<int, std::vector<iovec> > receive();
  void release(std::map<int, std::vector<iovec> > const& wrs_map);
};
}

#endif
