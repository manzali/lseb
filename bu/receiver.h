#ifndef BU_RECEIVER_H
#define BU_RECEIVER_H

#include "common/dataformat.h"
#include "common/shared_queue.h"

namespace lseb {

class Receiver {
  MetaDataRange m_metadata_range;
  DataRange m_data_range;
  SharedQueue<MetaDataRange>& m_ready_events_queue;
  SharedQueue<MetaDataRange>& m_sent_events_queue;
  std::vector<int> m_connection_ids;

 public:
  Receiver(MetaDataRange const& metadata_range, DataRange const& data_range,
         SharedQueue<MetaDataRange>& ready_events_queue,
         SharedQueue<MetaDataRange>& sent_events_queue,
         std::vector<int> const& connection_ids);

  void operator()();

};

}

#endif
