#include <algorithm>
#include <random>

#include <sys/uio.h>

#include "common/log.h"
#include "common/utility.h"

#if defined(TCP)
#include "transport/transport.h"
#include "ru/sender.h"
#endif

#if defined(RSOCKETS)
#include "transport/transport_rsockets.h"
#include "ru/sender_rsockets.h"
#endif

namespace lseb {

static std::mt19937 mt_rand(std::random_device { }());

Sender::Sender(
  MetaDataRange const& metadata_range,
  DataRange const& data_range,
  std::vector<int> const& connection_ids)
    :
      m_metadata_range(metadata_range),
      m_data_range(data_range),
      m_connection_ids(connection_ids),
      m_bu_id(0) {
}

size_t Sender::send(MultiEvents multievents) {

  size_t sent_bytes = 0;

  assert(multievents.size() != 0 && "Can't compute the module of zero!");
  int const offset = mt_rand() % multievents.size();
  m_bu_id += offset;

  for (auto it = std::begin(multievents); it != std::end(multievents); ++it) {

    auto const& p = *(advance_in_range(it, offset, multievents));
    std::vector<iovec> iov = create_iovec(p.first, m_metadata_range);
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

    std::vector<iovec> iov_data = create_iovec(p.second, m_data_range);
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

    ssize_t ret = lseb_write(
      m_connection_ids[m_bu_id % m_connection_ids.size()],
      iov);
    assert(ret >= 0 && static_cast<size_t>(ret) == (meta_load + data_load));

    sent_bytes += ret;
    ++m_bu_id;
  }
  m_bu_id = (m_bu_id - offset) % m_connection_ids.size();
  assert(m_bu_id >= 0 && "Wrong bu id");

  return sent_bytes;
}

}
