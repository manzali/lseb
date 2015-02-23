#ifndef RU_SENDER_H
#define RU_SENDER_H

#include "../common/dataformat.h"
#include "../common/shared_queue.h"

namespace lseb {

class Sender {
  MetaDataRange m_metadata_range;
  DataRange m_data_range;
  SharedQueue<MetaDataRange>& m_ready_events_queue;
  SharedQueue<MetaDataRange>& m_sent_events_queue;

 public:

  Sender(MetaDataRange const& metadata_range, DataRange const& data_range,
         SharedQueue<MetaDataRange>& ready_events_queue,
         SharedQueue<MetaDataRange>& sent_events_queue);

  void operator()(size_t bulk_size);

};

}

#endif
