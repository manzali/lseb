#include "bu/receiver.h"

#include <algorithm>

#include "common/log.h"

namespace lseb {

Receiver::Receiver(
  MetaDataRange const& metadata_range,
  DataRange const& data_range,
  size_t events_in_multievent,
  std::vector<BuConnectionId> const& connection_ids)
    :
      m_metadata_range(metadata_range),
      m_data_range(data_range),
      m_events_in_multievent(events_in_multievent),
      m_connection_ids(connection_ids) {
  // Prepare the vector of file descriptors to be polled
  std::transform(
    std::begin(connection_ids),
    std::end(connection_ids),
    std::back_inserter(m_poll_fds),
    [](BuConnectionId const& id) {return pollfd {id.socket, POLLIN, 0};});
}

size_t Receiver::receive(int timeout_ms) {

  size_t const multievent_size = m_events_in_multievent * sizeof(EventMetaData);

  int n_events = lseb_poll(m_poll_fds, timeout_ms);
  size_t read_bytes = 0;

  for (auto it = std::begin(m_poll_fds);
      n_events > 0 && it != std::end(m_poll_fds); ++it) {

    if (it->revents & POLLIN) {
      int const conn_index = std::distance(std::begin(m_poll_fds), it);
      ssize_t ret = lseb_read(m_connection_ids[conn_index]);
      assert(ret != -1);
      read_bytes += ret;
      n_events--;
    }
  }
  return read_bytes;
}

}
