#include "bu/receiver.h"

#include "common/log.h"
#include "common/frequency_meter.h"
#include "transport/transport.h"

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

  FrequencyMeter bandwith(1.0);

  size_t const events_in_multievent = std::distance(
      std::begin(m_metadata_range), std::end(m_metadata_range))
      / m_connection_ids.size();

  size_t const multievent_size = events_in_multievent * sizeof(EventMetaData);

  // Prepare the vector of file descriptors to be polled
  std::vector<pollfd> poll_fds;
  for (auto const& id : m_connection_ids) {
    poll_fds.push_back(pollfd { id, POLLIN, 0 });
  }

  while (true) {

    int nevents = lseb_poll(poll_fds, 0);

    for (auto it = poll_fds.begin(); nevents > 0 && it != poll_fds.end();
        ++it) {
      if (it->revents & POLLIN) {

        // Read metadata
        size_t const offset = std::distance(poll_fds.begin(), it)
            * events_in_multievent;
        MetaDataRange multievent_meta(
            m_metadata_range.begin() + offset,
            m_metadata_range.begin() + offset + events_in_multievent);

        ssize_t read_bytes = 0;
        while (static_cast<size_t>(read_bytes) != multievent_size) {
          ssize_t temp = lseb_read(it->fd,
                                   (char*) multievent_meta.begin() - read_bytes,
                                   multievent_size - read_bytes);
          assert(temp >= 0);
          read_bytes += temp;
        }

        LOG(DEBUG) << "Read " << read_bytes << " / " << multievent_size
                   << " bytes";

        assert(static_cast<size_t>(read_bytes) == multievent_size);

        bandwith.add(read_bytes);

        // Read data
        size_t data_load = 0;
        for (auto const& meta : multievent_meta) {
          data_load += meta.length;
        }

        read_bytes = 0;
        while (static_cast<size_t>(read_bytes) != data_load) {
          ssize_t temp = lseb_read(it->fd, m_data_range.begin() + read_bytes,
                                   data_load - read_bytes);
          assert(temp >= 0);
          read_bytes += temp;
        }

        LOG(DEBUG) << "Read " << read_bytes << " / " << data_load << " bytes";

        assert(static_cast<size_t>(read_bytes) == data_load);

        bandwith.add(read_bytes);

        nevents--;
      }
    }

    if (bandwith.check()) {
      LOG(INFO) << "Bandwith: " << bandwith.frequency() / std::giga::num * 8.
                << " Gb/s";
    }
  }

}

}
