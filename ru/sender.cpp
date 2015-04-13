#include <algorithm>
#include <random>

#include <cstring>
#include <sys/uio.h>

#include "common/log.h"
#include "common/utility.h"

#include "ru/sender.h"

namespace lseb {

static std::mt19937 mt_rand(std::random_device { }());

Sender::Sender(
  MetaDataRange const& metadata_range,
  DataRange const& data_range,
  std::vector<RuConnectionId> const& connection_ids)
    :
      m_metadata_range(metadata_range),
      m_data_range(data_range),
      m_connection_ids(connection_ids),
      m_next_bu(std::begin(m_connection_ids)) {

  // Registration
  for (auto& conn : m_connection_ids) {
    lseb_register(conn);
  }

}

size_t Sender::send(MultiEvents multievents) {

  assert(multievents.size() != 0 && "Can't compute the module of zero!");

  std::vector<
      std::pair<std::vector<RuConnectionId>::iterator, MultiEvents::iterator> > conn_iterators;

  for (auto it = std::begin(multievents); it != std::end(multievents); ++it) {
    conn_iterators.emplace_back(m_next_bu, it);
    if (++m_next_bu == std::end(m_connection_ids)) {
      m_next_bu = std::begin(m_connection_ids);
    }
  }

  size_t sent_bytes = 0;

  auto it = std::begin(conn_iterators);
  while (it != std::end(conn_iterators)) {

    auto& conn_it = it->first;
    if (lseb_poll(*conn_it)) {

      auto& multi_it = it->second;

      std::vector<iovec> iov = create_iovec(multi_it->first, m_metadata_range);
      size_t meta_load = std::accumulate(
        std::begin(iov),
        std::end(iov),
        0,
        [](size_t partial, iovec const& v) {
          return partial + v.iov_len;});
      LOG(DEBUG)
        << "Written for metadata "
        << meta_load
        << " bytes in "
        << iov.size()
        << " iovec";

      std::vector<iovec> iov_data = create_iovec(
        multi_it->second,
        m_data_range);
      size_t data_load = std::accumulate(
        std::begin(iov_data),
        std::end(iov_data),
        0,
        [](size_t partial, iovec const& v) {
          return partial + v.iov_len;});
      LOG(DEBUG)
        << "Written for data "
        << data_load
        << " bytes in "
        << iov_data.size()
        << " iovec";

      iov.insert(std::end(iov), std::begin(iov_data), std::end(iov_data));

      LOG(DEBUG) << "Sending iovec to connection id " << m_next_bu->socket;

      ssize_t ret = lseb_write(*conn_it, iov);
      assert(ret >= 0 && static_cast<size_t>(ret) == (meta_load + data_load));

      sent_bytes += ret;

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

  return sent_bytes;
}

}
