#include <algorithm>
#include <list>
#include <chrono>

#include <cstring>
#include <sys/uio.h>

#include "common/log.hpp"
#include "common/utility.h"
#include "common/handler_executor.hpp"

#include "ru/sender.h"

namespace lseb {

using DataVectorIter = std::vector<DataIov>::iterator;

struct SendingStruct {
  std::vector<RuConnectionId>::iterator ruConnectionIdIter;
  std::vector<DataVectorIter> dataVector;
  std::vector<DataVectorIter>::iterator dataVectorIter;
};

void poll_and_send(
  HandlerExecutor* executor,
  std::vector<SendingStruct>::iterator sending_vector_it) {
  auto& conn = *(sending_vector_it->ruConnectionIdIter);
  if (lseb_poll(conn)) {
    auto& iov_it = sending_vector_it->dataVectorIter;
    auto& fragment = **iov_it;
    ssize_t ret = lseb_write(conn, fragment);
    if (ret != -2) {
      size_t load = iovec_length(fragment);
      assert(static_cast<size_t>(ret) == load);
      LOG(DEBUG)
        << "Written "
        << load
        << " bytes in "
        << fragment.size()
        << " iovec and sending to connection id "
        << conn.socket;
      if (++iov_it == std::end(sending_vector_it->dataVector)) {
        return;
      }
    }
  }
  executor->post(std::bind(poll_and_send, executor, sending_vector_it));
}

Sender::Sender(std::vector<RuConnectionId> const& connection_ids)
    :
      m_connection_ids(connection_ids),
      m_next_bu(std::begin(m_connection_ids)),
      m_executor(2) {
  LOG(INFO) << "Waiting for synchronization...";
  for (auto& conn : m_connection_ids) {
    lseb_sync(conn);
  }
  LOG(INFO) << "Synchronization completed";
}

size_t Sender::send(
  std::vector<DataIov> data_iovecs,
  std::chrono::milliseconds ms_timeout) {

  // Create sending vector
  std::vector<SendingStruct> sending_vector(
    (data_iovecs.size() < m_connection_ids.size()) ?
      data_iovecs.size() :
      m_connection_ids.size());

  // Fill sending vector with multievents
  auto sending_list_it = std::begin(sending_vector);
  for (auto it = std::begin(data_iovecs); it != std::end(data_iovecs); ++it) {
    sending_list_it->dataVector.emplace_back(it);
    if (++sending_list_it == std::end(sending_vector)) {
      sending_list_it = std::begin(sending_vector);
    }
  }

  // Fill sending vector with connection ids and post the task
  for (auto it = std::begin(sending_vector); it != std::end(sending_vector);
      ++it) {
    it->ruConnectionIdIter = m_next_bu;
    it->dataVectorIter = std::begin(it->dataVector);
    if (++m_next_bu == std::end(m_connection_ids)) {
      m_next_bu = std::begin(m_connection_ids);
    }
    m_executor.post(std::bind(poll_and_send, &m_executor, it));
  }
  m_executor.wait();

  size_t sent_bytes = 0;
  for (auto& sending_struct : sending_vector) {
    for (auto iov_it = std::begin(sending_struct.dataVector);
        iov_it != sending_struct.dataVectorIter; ++iov_it) {
      sent_bytes += iovec_length(**iov_it);
    }
    if (sending_struct.dataVectorIter != std::end(sending_struct.dataVector)) {
      LOG(WARNING)
        << "Incompleted data send to connection "
        << sending_struct.ruConnectionIdIter->socket;
    }
  }
  return sent_bytes;
}

}
