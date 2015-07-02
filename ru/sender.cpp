#include <algorithm>
#include <list>

#include <cstring>
#include <sys/uio.h>

#include "common/log.hpp"
#include "common/utility.h"

#include "ru/sender.h"

namespace lseb {

Sender::Sender(std::vector<RuConnectionId> const& connection_ids)
    :
      m_connection_ids(connection_ids),
      m_next_bu(std::begin(m_connection_ids)) {
  LOG(INFO) << "Waiting for synchronization...";
  for (auto& conn : m_connection_ids) {
    lseb_sync(conn);
  }
  LOG(INFO) << "Synchronization completed";
}

size_t Sender::send(
  std::vector<DataIov> data_iovecs,
  std::chrono::milliseconds) {

  using DataVectorIter = std::vector<DataIov>::iterator;

  struct SendingStruct {
    std::vector<RuConnectionId>::iterator ruConnectionIdIter;
    std::vector<DataVectorIter> dataVector;
    std::vector<DataVectorIter>::iterator dataVectorIter;
  };

  size_t list_size =
      (data_iovecs.size() < m_connection_ids.size()) ?
        data_iovecs.size() :
        m_connection_ids.size();
  std::list<SendingStruct> sending_list(list_size);

  // Filling dataVector
  auto sending_list_it = std::begin(sending_list);
  for (auto it = std::begin(data_iovecs); it != std::end(data_iovecs); ++it) {
    sending_list_it->dataVector.emplace_back(it);
    if (++sending_list_it == std::end(sending_list)) {
      sending_list_it = std::begin(sending_list);
    }
  }

  // Setting ruConnectionIdIter and dataVector
  for (auto it = std::begin(sending_list); it != std::end(sending_list); ++it) {
    it->ruConnectionIdIter = m_next_bu;
    it->dataVectorIter = std::begin(it->dataVector);
    if (++m_next_bu == std::end(m_connection_ids)) {
      m_next_bu = std::begin(m_connection_ids);
    }
  }

  sending_list_it = select_randomly(
    std::begin(sending_list),
    std::end(sending_list));
  size_t written_bytes = 0;

  while (sending_list_it != std::end(sending_list)) {

    bool remove_it = false;
    auto& conn = *(sending_list_it->ruConnectionIdIter);

    if (lseb_poll(conn)) {

      auto& iov_it = sending_list_it->dataVectorIter;
      auto& iov = *iov_it;
      size_t load = iovec_length(*iov);

      LOG(DEBUG) << "Written " << load << " bytes in " << iov->size() << " iovec and sending to connection id " << conn
        .socket;

      ssize_t ret = lseb_write(conn, *iov);

      if (ret != -2) {
        assert(ret >= 0 && static_cast<size_t>(ret) == load);
        written_bytes += ret;
        if (++iov_it == std::end(sending_list_it->dataVector)) {
          remove_it = true;
        }
      }
    }

    if (remove_it) {
      sending_list_it = sending_list.erase(sending_list_it);
    } else {
      ++sending_list_it;
    }

    if (sending_list_it == std::end(sending_list)) {
      sending_list_it = std::begin(sending_list);
    }
  }

  return written_bytes;
}

}
