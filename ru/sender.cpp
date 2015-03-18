#include "ru/sender.h"

#include <random>
#include <algorithm>

#include <sys/uio.h>

#include "common/log.h"
#include "common/utility.h"
#include "common/frequency_meter.h"

#include "transport/transport.h"

namespace lseb {

Sender::Sender(MetaDataRange const& metadata_range, DataRange const& data_range,
               SharedQueue<MetaDataRange>& ready_events_queue,
               SharedQueue<MetaDataRange>& sent_events_queue,
               std::vector<int> const& connection_ids, size_t multievent_size)
    : m_metadata_range(metadata_range),
      m_data_range(data_range),
      m_ready_events_queue(ready_events_queue),
      m_sent_events_queue(sent_events_queue),
      m_connection_ids(connection_ids),
      m_multievent_size(multievent_size) {
}

void Sender::operator()() {

  FrequencyMeter bandwith(1.0);

  auto first_multievent_metadata = std::begin(m_metadata_range);
  auto first_multievent_data = std::begin(m_data_range);
  unsigned int generated_events = 0;
  BoundedInt bu_id(0, m_connection_ids.size() - 1);
  std::mt19937 mt_rand(std::random_device { }());
  std::vector<std::pair<MetaDataRange, DataRange> > multievents;

  while (true) {

    // Get ready events
    MetaDataRange metadata_subrange = m_ready_events_queue.pop();
    generated_events += distance_in_range(metadata_subrange, m_metadata_range);

    // Handle bulk submission and release events
    while (generated_events >= m_multievent_size) {

      // Create bulked metadata and data ranges

      MetaDataRange multievent_metadata(
          first_multievent_metadata,
          advance_in_range(first_multievent_metadata, m_multievent_size,
                           m_metadata_range));

      size_t data_length =
          accumulate_in_range(
              multievent_metadata,
              m_metadata_range,
              size_t(0),
              [](size_t partial, EventMetaData const& it) {return partial + it.length;});

      DataRange multievent_data(
          first_multievent_data,
          advance_in_range(first_multievent_data, data_length, m_data_range));

      multievents.push_back(
          std::make_pair(multievent_metadata, multievent_data));
      generated_events -= m_multievent_size;
      first_multievent_metadata = std::end(multievent_metadata);
      first_multievent_data = std::end(multievent_data);
    }

    if (!multievents.empty()) {

      LOG(DEBUG) << "Bulk size: " << multievents.size();

      int const offset = mt_rand() % multievents.size();

      bu_id += offset;
      for (auto it = std::begin(multievents); it != std::end(multievents);
          ++it) {

        auto const& p = *(advance_in_range(it, offset, multievents));
        std::vector<iovec> iov = create_iovec(p.first, m_metadata_range);
        size_t meta_load = std::accumulate(
            std::begin(iov),
            std::end(iov),
            0,
            [](size_t partial, iovec const& v) {
              return partial + v.iov_len;}
        );
        LOG(DEBUG) << "Written for metadata " << meta_load << " bytes in " << iov.size() << " iovec";

        std::vector<iovec> iov_data = create_iovec(p.second, m_data_range);
        size_t data_load = std::accumulate(
            std::begin(iov_data),
            std::end(iov_data),
            0,
            [](size_t partial, iovec const& v) {
              return partial + v.iov_len;}
        );
        LOG(DEBUG) << "Written for data " << data_load << " bytes in " << iov_data.size() << " iovec";

        iov.insert(std::end(iov), std::begin(iov_data), std::end(iov_data));

        ssize_t written_bytes = lseb_write(m_connection_ids[bu_id.get()], iov);
        //ssize_t written_bytes = (meta_load + data_load);

        assert(written_bytes >= 0 && static_cast<size_t>(written_bytes) == (meta_load + data_load));

        bandwith.add(meta_load);
        bandwith.add(data_load);

        ++bu_id;
      }
      bu_id -= offset;

      // Release all events
      m_sent_events_queue.push(
          MetaDataRange(std::begin(multievents.front().first),
                        std::end(multievents.back().first)));
      multievents.clear();
    }

    if (bandwith.check()) {
      LOG(INFO) << "Bandwith: " << bandwith.frequency() / std::giga::num * 8.
                << " Gb/s";
    }

  }
}

}
