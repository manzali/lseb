#include <memory>
#include <string>
#include <algorithm>
#include <thread>
#include <chrono>

#include "common/frequency_meter.h"
#include "log/log.hpp"
#include "common/dataformat.h"
#include "common/utility.h"

#include "bu/builder_unit.h"

namespace lseb {

BuilderUnit::BuilderUnit(
    int nodes,
    int bulk_size,
    int credits,
    int max_fragment_size,
    int id)
    : m_data_vect(nodes),
      m_bulk_size(bulk_size),
      m_credits(credits),
      m_max_fragment_size(max_fragment_size),
      m_id(id),
      m_data_ptr(
          new unsigned char[max_fragment_size * bulk_size * credits * nodes]) {
}

int BuilderUnit::read_data(int id) {
  auto& iov_vect = m_data_vect[id];
  int const old_size = iov_vect.size();
  auto& conn = *(m_connection_ids.at(id));
  std::vector<iovec> new_data = conn.poll_completed_recv();
  if (!new_data.empty()) {
    iov_vect.insert(
        std::end(iov_vect),
        std::begin(new_data),
        std::end(new_data));
  }
  return iov_vect.size() - old_size;
}

bool BuilderUnit::check_data() {
  uint64_t first_evt_id = pointer_cast<EventHeader>(
      m_data_vect[0].front().iov_base)->id;
  for (auto& data : m_data_vect) {
    uint64_t id = pointer_cast<EventHeader>(data.front().iov_base)->id;
    uint64_t flags = pointer_cast<EventHeader>(data.front().iov_base)->flags;
    uint64_t length = pointer_cast<EventHeader>(data.front().iov_base)->length;
    if (id != first_evt_id) {
      LOG_ERROR
        << "Remote event id ("
        << id
        << ") is different from the event id of the BU 0 ("
        << first_evt_id
        << ")";
      return false;
    }
    //if (flags >= m_endpoints.size()) {
    //  LOG(ERROR) << "Found wrong connection id: " << flags;
    //  return false;
    //}
    if (!length) {
      LOG_ERROR << "Found wrong length: " << length;
      return false;
    }
  }
  return true;
}

size_t BuilderUnit::release_data(int id, int n) {
  auto& iov_vect = m_data_vect[id];
  assert(iov_vect.size() >= n);
  std::vector<iovec> sub_vect(std::begin(iov_vect), std::begin(iov_vect) + n);
  // Erase iovec
  iov_vect.erase(std::begin(iov_vect), std::begin(iov_vect) + n);
  size_t const bytes = iovec_length(sub_vect);
  // Release iovec
  auto& conn = *(m_connection_ids.at(id));
  // Reset len of iovec
  for (auto& iov : sub_vect) {
    iov.iov_len = m_max_fragment_size * m_bulk_size;  // chunk size
    conn.post_recv(iov);
  }
  return bytes;
}

void BuilderUnit::connect(std::vector<Endpoint> const& endpoints) {

  // Connections

  Acceptor acceptor(m_credits);

  acceptor.listen(endpoints[m_id].hostname(), endpoints[m_id].port());

  LOG_INFO << "Builder Unit - Waiting for connections...";

  size_t const chunk_size = m_max_fragment_size * m_bulk_size;

  for (int i = 0; i < endpoints.size(); ++i) {

    // Create a temporary entry in the map with the local id
    auto p = m_connection_ids.emplace(
        std::make_pair(i, std::unique_ptr<Socket>(acceptor.accept())));
    auto& conn = *(m_connection_ids.at(i));

    unsigned char* base_data_ptr = m_data_ptr.get()
        + i * chunk_size * m_credits;
    conn.register_memory(base_data_ptr, chunk_size * m_credits);

    for (int j = 0; j < m_credits; ++j) {
      conn.post_recv({ base_data_ptr + j * chunk_size, chunk_size });
    }

    LOG_INFO
      << "Builder Unit - Connection established with ip "
      << conn.peer_hostname();
  }
  LOG_INFO << "Builder Unit - All connections established";
}

void BuilderUnit::run() {

  FrequencyMeter frequency(5.0);

  std::chrono::high_resolution_clock::time_point t_tot =
      std::chrono::high_resolution_clock::now();
  std::chrono::high_resolution_clock::time_point t_active;
  double active_time = 0;

  while (true) {

    bool active_flag = false;
    t_active = std::chrono::high_resolution_clock::now();

    // Acquire
    int min_wrs = m_credits;
    for (int i = 0; i < m_connection_ids.size(); ++i) {
      int read_wrs = read_data(i);
      if (read_wrs) {
        LOG_TRACE
          << "Builder Unit - Read "
          << read_wrs
          << " wrs from conn "
          << i;
        active_flag = true;
      }
      int const current_wrs = m_data_vect[i].size();
      min_wrs = (min_wrs < current_wrs) ? min_wrs : current_wrs;
    }

    if (min_wrs) {
      if (!check_data()) {
        throw std::runtime_error("Error checking data");
      }

      // Release
      for (int i = 0; i < m_connection_ids.size(); ++i) {
        size_t const bytes = release_data(i, min_wrs);
        LOG_TRACE
          << "Builder Unit - Released "
          << min_wrs
          << " wrs of conn "
          << i;
      }
      frequency.add(min_wrs * m_bulk_size * m_connection_ids.size());
    }

    if (active_flag) {
      active_time += std::chrono::duration<double>(
          std::chrono::high_resolution_clock::now() - t_active).count();
    }

    if (frequency.check()) {

      double tot_time = std::chrono::duration<double>(
          std::chrono::high_resolution_clock::now() - t_tot).count();

      LOG_INFO
        << "Builder Unit: "
        << frequency.frequency() / std::mega::num
        << " MHz - "
        << active_time / tot_time * 100.
        << " %";
      active_time = 0;
      t_tot = std::chrono::high_resolution_clock::now();
    }
  }

  LOG_DEBUG << "Builder Unit: exiting";
}
}
