#ifndef BU_RECEIVER_H
#define BU_RECEIVER_H

#include <vector>

#include "common/dataformat.h"
#include "transport/transport.h"

namespace lseb {

class Receiver {
  size_t m_events_in_multievent;
  std::vector<BuConnectionId> m_connection_ids;

 public:
  Receiver(
    size_t events_in_multievent,
    std::vector<BuConnectionId> const& connection_ids);
  size_t receive();
};

}

#endif
