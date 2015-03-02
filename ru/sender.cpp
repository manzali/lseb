#include "ru/sender.h"

#include <sys/uio.h>

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
    bulked_events += distance_in_range(metadata_subrange, m_metadata_range);

    // Handle bulk submission and release events
    while (bulked_events >= bulk_size) {

      // Create bulked metadata and data ranges

      MetaDataRange bulked_metadata(
          first_bulked_metadata,
          advance_in_range(first_bulked_metadata, bulk_size, m_metadata_range));

      MetaDataRange::iterator last_bulked_metadata = advance_in_range(
          bulked_metadata.begin(), bulk_size - 1, m_metadata_range);

      DataRange bulked_data(
          m_data_range.begin() + bulked_metadata.begin()->offset,
          m_data_range.begin() + last_bulked_metadata->offset
              + last_bulked_metadata->length);

      // Create iovec

      std::vector<iovec> iov = create_iovec(bulked_metadata, m_metadata_range);
      std::vector<iovec> iov_data = create_iovec(bulked_data, m_data_range);

      iov.insert(iov.end(), iov_data.begin(), iov_data.end());

      for (auto& i : iov) {
        LOG(DEBUG) << i.iov_base << "\t[" << i.iov_len << "]";
      }

      size_t bulk_load = 0;
      std::for_each(iov.begin(), iov.end(),
                    [&](iovec const& i) {bulk_load += i.iov_len;});

      m_sent_events_queue.push(bulked_metadata);
      bulked_events -= bulk_size;
      first_bulked_metadata = bulked_metadata.end();
    }
  }
}

}
