#include <algorithm>
#include <list>

#include <cstring>
#include <sys/uio.h>

#include "common/log.h"
#include "common/utility.h"

#include "ru/sender.h"

namespace lseb {

static size_t iovec_length(std::vector<iovec> const& iov) {
  return std::accumulate(
    std::begin(iov),
    std::end(iov),
    0,
    [](size_t partial, iovec const& v) {
      return partial + v.iov_len;});
}

Sender::Sender(
  std::vector<RuConnectionId> const& connection_ids,
  size_t max_sending_size)
    :
      m_connection_ids(connection_ids),
      m_next_bu(std::begin(m_connection_ids)),
      m_max_sending_size(max_sending_size) {

  // Registration
  for (auto& conn : m_connection_ids) {
    lseb_sync(conn);
  }

}

size_t Sender::send(std::vector<DataIov> data_iovecs) {

  std::list<
      std::pair<std::vector<RuConnectionId>::iterator,
          std::vector<DataIov>::iterator> > conn_iterators;

  for (auto it = std::begin(data_iovecs); it != std::end(data_iovecs); ++it) {
    conn_iterators.emplace_back(m_next_bu, it);
    if (++m_next_bu == std::end(m_connection_ids)) {
      m_next_bu = std::begin(m_connection_ids);
    }
  }

  size_t written_bytes = 0;

  auto it = std::begin(conn_iterators);
  /*select_randomly(
   std::begin(conn_iterators),
   std::next(std::begin(conn_iterators), m_connection_ids.size()));
   */

  while (it != std::end(conn_iterators)) {

    auto& conn_it = it->first;
    if (lseb_poll(*conn_it)) {

      auto& iov = it->second;
      size_t load = iovec_length(*iov);

      LOG(DEBUG) << "Written " << load << " bytes in " << iov->size() << " iovec and sending to connection id " << conn_it
        ->socket;

      assert(
        load <= m_max_sending_size && "Trying to send a buffer bigger than the receiver one");

      ssize_t ret = lseb_write(*conn_it, *iov);

      assert(ret >= 0 && static_cast<size_t>(ret) == load);

      written_bytes += ret;

      it = conn_iterators.erase(it);
    } else {
      ++it;
    }
    if (it == std::end(conn_iterators) || std::distance(
      std::begin(conn_iterators),
      it) == m_connection_ids.size()) {
      it = std::begin(conn_iterators);
    }
  }

  return written_bytes;
}

}
