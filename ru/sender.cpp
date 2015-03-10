#include "ru/sender.h"

#include <random>
#include <algorithm>

#include <sys/uio.h>

#include "common/log.h"
#include "common/utility.h"
#include "common/frequency_meter.h"

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

  auto first_bulked_metadata = std::begin(m_metadata_range);
  auto first_bulked_data = std::begin(m_data_range);
  unsigned int generated_events = 0;
  int bu_id = 0;
  std::mt19937 mt_rand(std::random_device { }());
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

      size_t data_length =
          accumulate_in_range(
              bulked_metadata,
              m_metadata_range,
              size_t(0),
              [](size_t partial, EventMetaData const& it) {return partial + it.length;});

      DataRange bulked_data(
          first_bulked_data,
          advance_in_range(first_bulked_data, data_length, m_data_range));

      bulked_events.push_back(std::make_pair(bulked_metadata, bulked_data));
      generated_events -= m_bulk_size;
      first_bulked_metadata = std::end(bulked_metadata);
      first_bulked_data = std::end(bulked_data);
    }

    if (!bulked_events.empty()) {

      LOG(INFO) << "Bulk size: " << bulked_events.size();

      for (size_t s = bulked_events.size(), i = mt_rand() % s, e = i + s;
          i != e; ++i) {

        auto const& p = *(std::begin(bulked_events) + i % s);
        std::vector<iovec> iov = create_iovec(p.first, m_metadata_range);
        std::vector<iovec> iov_data = create_iovec(p.second, m_data_range);
        iov.insert(std::end(iov), std::begin(iov_data), std::end(iov_data));

        size_t bulk_load = 0;
        std::for_each(std::begin(iov), std::end(iov), [&](iovec const& i) {
          bulk_load += i.iov_len; LOG(DEBUG) << i.iov_base << "\t["
          << i.iov_len << "]";});

        LOG(DEBUG) << "Sending " << bulk_load << " bytes to "
                   << m_endpoints[(bu_id + i % s) % m_endpoints.size()];

      }

      // Release all events
      m_sent_events_queue.push(
          MetaDataRange(std::begin(bulked_events.front().first),
                        std::end(bulked_events.back().first)));

      bu_id = (bu_id + bulked_events.size()) % m_endpoints.size();
      bulked_events.clear();
    }
  }
}

}
