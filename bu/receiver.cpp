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

        ssize_t read_bytes = 0;
        while (static_cast<size_t>(read_bytes) != m_multievent_size) {
          ssize_t temp = lseb_read(it->fd,
                                   (char*) multievent_meta.begin() - read_bytes,
                                   m_multievent_size - read_bytes);
          assert(temp >= 0);
          read_bytes += temp;
        }

        LOG(DEBUG) << "Read " << read_bytes << " / " << m_multievent_size
                   << " bytes";

        assert(static_cast<size_t>(read_bytes) == m_multievent_size);

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
