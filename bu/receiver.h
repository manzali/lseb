#ifndef BU_RECEIVER_H
#define BU_RECEIVER_H

#include <sys/poll.h>

#include "common/dataformat.h"

namespace lseb {

class Receiver {
  MetaDataRange m_metadata_range;
  DataRange m_data_range;
  size_t m_events_in_multievent;
  size_t m_multievent_size;
  std::vector<pollfd> m_poll_fds;

 public:
  Receiver(MetaDataRange const& metadata_range, DataRange const& data_range,
           size_t events_in_multievent, std::vector<int> const& connection_ids);

  void operator()();

};

}

#endif
