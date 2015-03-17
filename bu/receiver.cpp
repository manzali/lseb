#include "bu/receiver.h"

#include "common/log.h"

namespace lseb {

Receiver::Receiver(MetaDataRange const& metadata_range,
                   DataRange const& data_range,
                   SharedQueue<MetaDataRange>& ready_events_queue,
                   SharedQueue<MetaDataRange>& sent_events_queue,
                   std::vector<int> const& connection_ids)
    : m_metadata_range(metadata_range),
      m_data_range(data_range),
      m_ready_events_queue(ready_events_queue),
      m_sent_events_queue(sent_events_queue),
      m_connection_ids(connection_ids) {
}

void Receiver::operator()() {

  for (auto const& conn_id : m_connection_ids) {
    LOG(DEBUG) << conn_id;
  }

  while (true) {

    ;

  }

}

}
