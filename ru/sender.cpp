#include "ru/sender.h"

#include <ratio>

#include "common/log.h"
#include "common/utility.h"

namespace lseb {

Sender::Sender(MetaDataRange const& metadata_range, DataRange const& data_range,
               SharedQueue<MetaDataRange>& ready_events_queue,
               SharedQueue<MetaDataRange>& sent_events_queue)
    : m_metadata_range(metadata_range),
      m_data_range(data_range),
      m_ready_events_queue(ready_events_queue),
      m_sent_events_queue(sent_events_queue) {
}

void Sender::operator()(size_t bulk_size) {

  MetaDataRange::iterator first_bulked_metadata = m_metadata_range.begin();
  size_t bulked_events = 0;

  while (true) {
    // Get ready events
    MetaDataRange metadata_subrange = m_ready_events_queue.pop();
    size_t const generated_events = distance_in_range(metadata_subrange,
                                                      m_metadata_range);

    if (generated_events) {

      MetaDataRange::iterator before_last_metadata = advance_in_range(
          metadata_subrange.begin(), generated_events - 1, m_metadata_range);

      DataRange data_subrange(
          m_data_range.begin() + metadata_subrange.begin()->offset,
          m_data_range.begin() + before_last_metadata->offset
              + before_last_metadata->length);

      bulked_events += generated_events;

      // Handle bulk submission and release events
      while (bulked_events >= bulk_size) {
        MetaDataRange::iterator last_bulked_metadata = advance_in_range(
            first_bulked_metadata, bulk_size, m_metadata_range);

        MetaDataRange bulked_metadata(first_bulked_metadata,
                                      last_bulked_metadata);

        MetaDataRange::iterator before_last_bulked_metadata = advance_in_range(
            first_bulked_metadata, bulk_size - 1, m_metadata_range);

        size_t bulk_load = distance_in_range(
            MetaDataRange(first_bulked_metadata, last_bulked_metadata),
            m_metadata_range) * sizeof(EventMetaData);

        DataRange bulked_data(
            m_data_range.begin() + first_bulked_metadata->offset,
            m_data_range.begin() + before_last_bulked_metadata->offset
                + before_last_bulked_metadata->length);

        bulk_load += distance_in_range(bulked_data, m_data_range);

        LOG(DEBUG)
            << "Sending events from "
            << first_bulked_metadata->id
            << " to "
            << before_last_bulked_metadata->id
            << " ("
            << bulk_load
            << " bytes)"
            << "\nMetaData Bulk ["
            << bulked_metadata.begin()
            << ", "
            << bulked_metadata.end()
            << ((bulked_metadata.begin() > bulked_metadata.end()) ?
                "]\tMETADATA WRAP" : "]")
            << "\nData Bulk ["
            << (void*) bulked_data.begin()
            << ", "
            << (void*) bulked_data.end()
            << ((bulked_data.begin() > bulked_data.end()) ?
                "]\t\tDATA WRAP" : "]");

        m_sent_events_queue.push(bulked_metadata);
        bulked_events -= bulk_size;
        first_bulked_metadata = last_bulked_metadata;
      }
    }
  }
}

}
