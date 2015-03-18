#include "bu/receiver.h"

#include "common/log.h"
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
  std::transform(
      std::begin(connection_ids),
      std::end(connection_ids),
      std::back_inserter(m_poll_fds),
      [](int id) {return pollfd {id, POLLIN, 0};}
  );
}

size_t Receiver::receive(int timeout_ms){

  int nevents = lseb_poll(m_poll_fds, timeout_ms);
  size_t read_bytes = 0;

  for (auto it = m_poll_fds.begin(); nevents > 0 && it != m_poll_fds.end();
      ++it) {
    if (it->revents & POLLIN) {

      // Read metadata
      size_t const offset = std::distance(m_poll_fds.begin(), it)
          * m_events_in_multievent;
      MetaDataRange multievent_meta(
          m_metadata_range.begin() + offset,
          m_metadata_range.begin() + offset + m_events_in_multievent);

      ssize_t read_meta = lseb_read(it->fd, multievent_meta.begin(), m_multievent_size);
      assert(read_meta >= 0 && static_cast<size_t>(read_meta) == m_multievent_size);

      LOG(DEBUG) << "Read " << read_meta << " / " << m_multievent_size
                 << " bytes";

      // Read data
      size_t data_load = std::accumulate(
          std::begin(multievent_meta),
          std::end(multievent_meta),
          0,
          [](size_t partial, EventMetaData const& meta) {
            return partial + meta.length;
          }
      );

      ssize_t read_data= lseb_read(it->fd, m_data_range.begin(), data_load);
      assert(read_data >= 0 && static_cast<size_t>(read_data) == data_load);

      LOG(DEBUG) << "Read " << read_data << " / " << data_load << " bytes";

      read_bytes += read_meta;
      read_bytes += read_data;

      nevents--;
    }
  }
  return read_bytes;
}

}
