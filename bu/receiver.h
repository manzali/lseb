#ifndef BU_RECEIVER_H
#define BU_RECEIVER_H

#include <vector>

#include <sys/poll.h>

#include "common/dataformat.h"

#include "transport/transport.h"

namespace lseb {

class Receiver {
  MetaDataRange m_metadata_range;
  DataRange m_data_range;
  size_t m_events_in_multievent;
  std::vector<BuConnectionId> m_connection_ids;
  std::vector<pollfd> m_poll_fds;

 public:
  Receiver(
    MetaDataRange const& metadata_range,
    DataRange const& data_range,
    size_t events_in_multievent,
    std::vector<BuConnectionId> const& connection_ids);
  size_t receive(int timeout_ms);
};

}

#endif
