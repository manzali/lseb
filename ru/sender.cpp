
#include "common/log.hpp"
#include "common/utility.h"

#include "ru/sender.h"

namespace lseb {

Sender::Sender(std::vector<RuConnectionId> const& connection_ids)
    :
      m_connection_ids(connection_ids),
      m_next_bu(std::begin(m_connection_ids)) {
}

size_t Sender::send(int conn_id, std::vector<DataIov> const& data_iovecs) {
  size_t bytes_sent = 0;
  for (auto const& data_iov : data_iovecs) {
    size_t load = iovec_length(data_iov);

    LOG(DEBUG)
      << "Written "
      << load
      << " bytes in "
      << data_iov.size()
      << " iovec";

    size_t ret = lseb_write(m_connection_ids[conn_id], data_iov);
    assert(static_cast<size_t>(ret) == load);
    bytes_sent += ret;
  }
  return bytes_sent;
}

}
