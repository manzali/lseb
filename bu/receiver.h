#ifndef BU_RECEIVER_H
#define BU_RECEIVER_H

#include <vector>

#include "common/dataformat.h"
#include "common/timer.h"
#include "common/frequency_meter.h"

#include "transport/transport.h"

namespace lseb {

class Receiver {
  uint64_t m_expected_event_id;
  uint64_t m_events_step;
  size_t m_events_in_multievent;
  std::vector<BuConnectionId> m_connection_ids;
  Timer m_read_timer;
  Timer m_recv_timer;
  FrequencyMeter m_bandwith;

 public:
  Receiver(
    int bu_id,
    size_t total_bu,
    size_t events_in_multievent,
    std::vector<BuConnectionId> const& connection_ids);
  size_t receive();
};

}

#endif
