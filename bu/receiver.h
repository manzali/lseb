#ifndef BU_RECEIVER_H
#define BU_RECEIVER_H

#include <vector>

#include "common/dataformat.h"
#include "common/timer.h"
#include "common/frequency_meter.h"

#include "transport/transport.h"

namespace lseb {

class Receiver {
  std::vector<BuConnectionId> m_connection_ids;
  Timer m_read_timer;
  Timer m_recv_timer;
  FrequencyMeter m_bandwith;

 public:
  Receiver(std::vector<BuConnectionId> const& connection_ids);
  size_t receive();
};

}

#endif
