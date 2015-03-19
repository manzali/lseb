#include "ru/sender.h"

#include <algorithm>

#include <sys/uio.h>

#include "common/log.h"
#include "common/utility.h"

#include "transport/transport.h"

namespace lseb {

Sender::Sender(MetaDataRange const& metadata_range, DataRange const& data_range,
               std::vector<int> const& connection_ids, size_t multievent_size)
    : m_metadata_range(metadata_range),
      m_current_metadata(std::begin(m_metadata_range)),
      m_data_range(data_range),
      m_current_data(std::begin(m_data_range)),
      m_connection_ids(connection_ids),
      m_multievent_size(multievent_size),
      m_rand(std::random_device { }()),
      m_generated_events(0),
      m_bu_id(0) {
}

MultiEvents Sender::add(MetaDataRange metadata_range) {

  MultiEvents multievents;
  m_generated_events += distance_in_range(metadata_range, m_metadata_range);

  // Handle bulk submission and release events
  while (m_generated_events >= m_multievent_size) {

    // Create bulked metadata and data ranges
    MetaDataRange multievent_metadata(
        m_current_metadata,
        advance_in_range(m_current_metadata, m_multievent_size,
                         m_metadata_range));
    m_current_metadata = std::end(multievent_metadata);

    size_t data_length =
        accumulate_in_range(
            multievent_metadata,
            m_metadata_range,
            size_t(0),
            [](size_t partial, EventMetaData const& it) {return partial + it.length;});

    DataRange multievent_data(
        m_current_data,
        advance_in_range(m_current_data, data_length, m_data_range));
    m_current_data = std::end(multievent_data);

    multievents.push_back(std::make_pair(multievent_metadata, multievent_data));
    m_generated_events -= m_multievent_size;
  }
  return multievents;
}

size_t Sender::send(MultiEvents multievents) {

  size_t sent_bytes = 0;

  assert(multievents.size() != 0 && "Can't compute the module of zero!");
  int const offset = m_rand() % multievents.size();
  m_bu_id += offset;

  for (auto it = std::begin(multievents); it != std::end(multievents); ++it) {

    auto const& p = *(advance_in_range(it, offset, multievents));
    std::vector<iovec> iov = create_iovec(p.first, m_metadata_range);
    size_t meta_load = std::accumulate(std::begin(iov), std::end(iov), 0,
                                       [](size_t partial, iovec const& v) {
                                         return partial + v.iov_len;});
    LOG(DEBUG) << "Written for metadata " << meta_load << " bytes in "
               << iov.size() << " iovec";

    std::vector<iovec> iov_data = create_iovec(p.second, m_data_range);
    size_t data_load = std::accumulate(std::begin(iov_data), std::end(iov_data),
                                       0, [](size_t partial, iovec const& v) {
                                         return partial + v.iov_len;});
    LOG(DEBUG) << "Written for data " << data_load << " bytes in "
               << iov_data.size() << " iovec";

    iov.insert(std::end(iov), std::begin(iov_data), std::end(iov_data));

    ssize_t ret = lseb_write(
        m_connection_ids[m_bu_id % m_connection_ids.size()], iov);
    assert(ret >= 0 && static_cast<size_t>(ret) == (meta_load + data_load));

    sent_bytes += ret;
    ++m_bu_id;
  }
  m_bu_id = (m_bu_id - offset) % m_connection_ids.size();
  assert(m_bu_id >= 0 && "Wrong bu id");

  return sent_bytes;
}

}
