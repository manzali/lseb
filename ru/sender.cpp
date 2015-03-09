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

  auto first_bulked_metadata = m_metadata_range.begin();
  auto first_bulked_data = m_data_range.begin();
  size_t generated_events = 0;
  size_t bu_id = 0;
  std::mt19937 mt_rand(
      std::chrono::high_resolution_clock::now().time_since_epoch().count());

  std::vector<std::pair<MetaDataRange, DataRange> > bulked_events;

  FrequencyMeter memory_throughput(5);
  FrequencyMeter events_frequency(5);

  while (true) {

    // Get ready events
    MetaDataRange metadata_subrange = m_ready_events_queue.pop();
    size_t temp = distance_in_range(metadata_subrange, m_metadata_range);
    events_frequency.add(temp);
    generated_events += temp;
    // Handle bulk submission and release events
    while (generated_events >= m_bulk_size) {

      // Create bulked metadata and data ranges

      MetaDataRange bulked_metadata(
          first_bulked_metadata,
          advance_in_range(first_bulked_metadata, m_bulk_size,
                           m_metadata_range));

      size_t length = 0;

      if (bulked_metadata.begin() < bulked_metadata.end()) {
        for (auto it = bulked_metadata.begin();
            it != bulked_metadata.end(); ++it) {
          length += it->length;
        }
      } else {
        for (auto it = bulked_metadata.begin();
            it != m_metadata_range.end(); ++it) {
          length += it->length;
        }
        for (auto it = m_metadata_range.begin();
            it != bulked_metadata.end(); ++it) {
          length += it->length;
        }
      }

      DataRange bulked_data(
          first_bulked_data,
          advance_in_range(first_bulked_data, length, m_data_range));

      bulked_events.push_back(std::make_pair(bulked_metadata, bulked_data));
      generated_events -= m_bulk_size;
      first_bulked_metadata = bulked_metadata.end();
      first_bulked_data = bulked_data.end();
    }

    if (!bulked_events.empty()) {

      //LOG(INFO) << "Bulk size: " << bulked_events.size();

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
/*
        LOG(INFO) << "Sending " << bulk_load << " bytes to "
                  << m_endpoints[(bu_id + i % s) % m_endpoints.size()];
*/

        memory_throughput.add(bulk_load);
      }

      // Release all events
      m_sent_events_queue.push(
          MetaDataRange(bulked_events.front().first.begin(),
                        bulked_events.back().first.end()));

      bu_id = (bu_id + bulked_events.size()) % m_endpoints.size();
      bulked_events.clear();
    }

    if (memory_throughput.check()) {
      LOG(INFO) << "Throughput on memory is "
                << memory_throughput.frequency() / std::giga::num * 8.
                << " Gb/s";
    }
    if (events_frequency.check()) {
      LOG(INFO) << "Real events generation frequency is "
                << events_frequency.frequency() / std::mega::num << " MHz";
    }
  }
}

}
