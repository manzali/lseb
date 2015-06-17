#include <cstring>
#include <sys/uio.h>

#include "common/log.hpp"
#include "common/utility.h"
#include "common/handler_executor.hpp"

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

void handle_single_conn(
  std::shared_ptr<HandlerExecutor> const& executor,
  RuConnectionId& conn,
  DataIov const& fragment) {
  if (lseb_poll(conn)) {
    ssize_t ret = lseb_write(conn, fragment);
    size_t load = iovec_length(fragment);
    assert(ret >= 0 && static_cast<size_t>(ret) == load);
  } else {
    executor->post(
      conn.socket,
      std::bind(handle_single_conn, executor, conn, fragment));
  }
}

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

size_t Sender::send(std::vector<DataIov> data_iovecs) {

  auto executor = std::make_shared<HandlerExecutor>(2);
  size_t written_bytes = 0;

  for (auto& fragment : data_iovecs) {
    auto& conn = *m_next_bu;
    executor->post(
      conn.socket,
      std::bind(handle_single_conn, executor, conn, fragment));
    written_bytes += iovec_length(fragment);
    if (++m_next_bu == std::end(m_connection_ids)) {
      m_next_bu = std::begin(m_connection_ids);
    }
  }

  executor->stop();

  return written_bytes;
}

}
