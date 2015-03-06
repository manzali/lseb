#include "ru/sender.h"

#include <random>

#include <sys/uio.h>

#include "common/log.h"
#include "common/utility.h"

namespace lseb {

Sender::Sender(MetaDataRange const& metadata_range, DataRange const& data_range,
               SharedQueue<MetaDataRange>& ready_events_queue,
               SharedQueue<MetaDataRange>& sent_events_queue,
               Endpoints const& endpoints, size_t bulk_size)
    : m_metadata_range(metadata_range),
      m_data_range(data_range),
      m_ready_events_queue(ready_events_queue),
      m_sent_events_queue(sent_events_queue),
      m_endpoints(endpoints),
      m_bulk_size(bulk_size) {
}

void Sender::operator()() {

  auto first_bulked_metadata = m_metadata_range.begin();
  size_t generated_events = 0;
  size_t bu_id = 0;
  std::mt19937 mt_rand(
      std::chrono::high_resolution_clock::now().time_since_epoch().count());

  std::vector<std::pair<MetaDataRange, DataRange> > bulked_events;

  while (true) {

    // Get ready events
    MetaDataRange metadata_subrange = m_ready_events_queue.pop();
    generated_events += distance_in_range(metadata_subrange, m_metadata_range);

    // Handle bulk submission and release events
    while (generated_events >= m_bulk_size) {

      // Create bulked metadata and data ranges

      MetaDataRange bulked_metadata(
          first_bulked_metadata,
          advance_in_range(first_bulked_metadata, m_bulk_size,
                           m_metadata_range));

      auto last_bulked_metadata = advance_in_range(bulked_metadata.begin(),
                                                   m_bulk_size - 1,
                                                   m_metadata_range);

      DataRange bulked_data(
          m_data_range.begin() + bulked_metadata.begin()->offset,
          m_data_range.begin() + last_bulked_metadata->offset
              + last_bulked_metadata->length);

      bulked_events.push_back(std::make_pair(bulked_metadata, bulked_data));
      generated_events -= m_bulk_size;
      first_bulked_metadata = bulked_metadata.end();
    }

    if (!bulked_events.empty()) {

      LOG(INFO) << "Bulk size: " << bulked_events.size();

      for (size_t s = bulked_events.size(), i = mt_rand() % s, e = i + s;
          i != e; ++i) {

        auto it = bulked_events.begin() + i % s;

        std::vector<iovec> iov = create_iovec(it->first, m_metadata_range);
        std::vector<iovec> iov_data = create_iovec(it->second, m_data_range);
        iov.insert(iov.end(), iov_data.begin(), iov_data.end());

        size_t bulk_load = 0;
        std::for_each(iov.begin(), iov.end(), [&](iovec const& i) {
          bulk_load += i.iov_len; LOG(DEBUG) << i.iov_base << "\t["
          << i.iov_len << "]";});

        LOG(INFO) << "Sending " << bulk_load << " bytes to "
                  << m_endpoints[(bu_id + i % s) % m_endpoints.size()];
      }

      // Release all events
      m_sent_events_queue.push(
          MetaDataRange(bulked_events.front().first.begin(),
                        bulked_events.back().first.end()));

      bu_id = (bu_id + bulked_events.size()) % m_endpoints.size();
      bulked_events.clear();
    }
  }
}

}
