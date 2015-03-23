#include <algorithm>

#include "common/log.h"

#if defined(TCP)
#include "transport/transport.h"
#include "bu/receiver.h"
#endif

#if defined(RSOCKETS)
#include "transport/transport_rsockets.h"
#include "bu/receiver_rsockets.h"
#endif

namespace lseb {

Receiver::Receiver(
  MetaDataRange const& metadata_range,
  DataRange const& data_range,
  size_t events_in_multievent,
  std::vector<int> const& connection_ids)
    :
      m_metadata_range(metadata_range),
      m_data_range(data_range),
      m_events_in_multievent(events_in_multievent) {
  // Prepare the vector of file descriptors to be polled
  std::transform(
    std::begin(connection_ids),
    std::end(connection_ids),
    std::back_inserter(m_poll_fds),
    [](int id) {return pollfd {id, POLLIN, 0};});
}

size_t Receiver::receive(int timeout_ms) {

  size_t const multievent_size = m_events_in_multievent * sizeof(EventMetaData);

  int nevents = lseb_poll(m_poll_fds, timeout_ms);
  size_t read_bytes = 0;

  for (auto it = std::begin(m_poll_fds);
      nevents > 0 && it != std::end(m_poll_fds); ++it) {

    if (it->revents & POLLIN) {

      // Read metadata
      size_t const offset =
        std::distance(std::begin(m_poll_fds), it) * m_events_in_multievent;

      assert(
        std::begin(m_metadata_range) + offset + m_events_in_multievent <= std::end(
          m_metadata_range));

      MetaDataRange multievent_meta(
        std::begin(m_metadata_range) + offset,
        std::begin(m_metadata_range) + offset + m_events_in_multievent);

      ssize_t read_meta = lseb_read(
        it->fd,
        std::begin(multievent_meta),
        multievent_size);
      assert(
        read_meta >= 0 && static_cast<size_t>(read_meta) == multievent_size);

      // Read data
      size_t data_load = std::accumulate(
        std::begin(multievent_meta),
        std::end(multievent_meta),
        0,
        [](size_t partial, EventMetaData const& meta) {
          return partial + meta.length;
        });

      ssize_t read_data = lseb_read(
        it->fd,
        std::begin(m_data_range),
        data_load);
      assert(read_data >= 0 && static_cast<size_t>(read_data) == data_load);

      read_bytes += read_meta;
      read_bytes += read_data;

      nevents--;
    }
  }
  return read_bytes;
}

}
