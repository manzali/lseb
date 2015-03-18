#include "bu/receiver.h"

#include "common/log.h"
#include "common/frequency_meter.h"
#include "transport/transport.h"

namespace lseb {

Receiver::Receiver(MetaDataRange const& metadata_range,
                   DataRange const& data_range, size_t events_in_multievent,
                   std::vector<int> const& connection_ids)
    : m_metadata_range(metadata_range),
      m_data_range(data_range),
      m_events_in_multievent(events_in_multievent),
      m_multievent_size(events_in_multievent * sizeof(EventMetaData)) {
  // Prepare the vector of file descriptors to be polled
  for (auto const& id : connection_ids) {
    m_poll_fds.push_back(pollfd { id, POLLIN, 0 });
  }
}

void Receiver::operator()() {

  FrequencyMeter bandwith(1.0);

  while (true) {

    int nevents = lseb_poll(m_poll_fds, 0);

    for (auto it = m_poll_fds.begin(); nevents > 0 && it != m_poll_fds.end();
        ++it) {
      if (it->revents & POLLIN) {

        // Read metadata
        size_t const offset = std::distance(m_poll_fds.begin(), it)
            * m_events_in_multievent;
        MetaDataRange multievent_meta(
            m_metadata_range.begin() + offset,
            m_metadata_range.begin() + offset + m_events_in_multievent);

        ssize_t read_bytes = lseb_read(it->fd, multievent_meta.begin(), m_multievent_size);
        assert(read_bytes >= 0 && static_cast<size_t>(read_bytes) == m_multievent_size);

        LOG(DEBUG) << "Read " << read_bytes << " / " << m_multievent_size
                   << " bytes";

        bandwith.add(read_bytes);

        // Read data
        size_t data_load = std::accumulate(
            std::begin(multievent_meta),
            std::end(multievent_meta),
            0,
            [](size_t partial, EventMetaData const& meta) {
              return partial + meta.length;
            }
        );

        read_bytes = lseb_read(it->fd, m_data_range.begin(), data_load);
        assert(read_bytes >= 0 && static_cast<size_t>(read_bytes) == data_load);

        LOG(DEBUG) << "Read " << read_bytes << " / " << data_load << " bytes";

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
