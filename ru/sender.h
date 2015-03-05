#ifndef RU_SENDER_H
#define RU_SENDER_H

#include "common/dataformat.h"
#include "common/shared_queue.h"
#include "common/endpoints.h"

namespace lseb {

class Sender {
  MetaDataRange m_metadata_range;
  DataRange m_data_range;
  SharedQueue<MetaDataRange>& m_ready_events_queue;
  SharedQueue<MetaDataRange>& m_sent_events_queue;
  Endpoints m_endpoints;
  size_t m_bulk_size;
  size_t m_ru_id;

 public:

  Sender(MetaDataRange const& metadata_range, DataRange const& data_range,
         SharedQueue<MetaDataRange>& ready_events_queue,
         SharedQueue<MetaDataRange>& sent_events_queue,
         Endpoints const& endpoints, size_t bulk_size, size_t ru_id);

  void operator()();

};

}

#endif
