#include <memory>
#include <string>
#include <algorithm>
#include <map>
#include <thread>
#include <chrono>

#include "common/frequency_meter.h"
#include "common/log.hpp"
#include "common/dataformat.h"
#include "common/utility.h"

#include "bu/builder_unit.h"

namespace lseb {

namespace {

int find_endpoint_id(
  std::vector<Endpoint> const& endpoints,
  std::string const& address) {
  auto it = std::find_if(
    std::begin(endpoints),
    std::end(endpoints),
    [&address](Endpoint const& ep) {return ep.hostname() == address;});
  return
      (it == std::end(endpoints)) ?
        -1 :
        std::distance(std::begin(endpoints), it);
}

}

BuilderUnit::BuilderUnit(
  boost::lockfree::spsc_queue<iovec>& free_local_data,
  boost::lockfree::spsc_queue<iovec>& ready_local_data,
  std::vector<Endpoint> const& endpoints,
  int bulk_size,
  int credits,
  int max_fragment_size,
  int id)
    :
      m_free_local_queue(free_local_data),
      m_ready_local_queue(ready_local_data),
      m_endpoints(endpoints),
      m_data_vect(endpoints.size()),
      m_bulk_size(bulk_size),
      m_credits(credits),
      m_max_fragment_size(max_fragment_size),
      m_id(id) {
}

int BuilderUnit::read_data(int id) {
  auto& iov_vect = m_data_vect[id];
  int const old_size = iov_vect.size();
  if (id != m_id) {
    auto& conn = *(m_connection_ids.at(id));
    std::vector<iovec> new_data = conn.pop_completed();
    if (!new_data.empty()) {
      iov_vect.insert(
        std::end(iov_vect),
        std::begin(new_data),
        std::end(new_data));
    }
  } else {
    iovec iov;
    while (m_ready_local_queue.pop(iov)) {
      iov_vect.push_back(iov);
    }
  }
  return iov_vect.size() - old_size;
}

bool BuilderUnit::check_data() {
  uint64_t local_evt_id = pointer_cast<EventHeader>(
    m_data_vect[m_id].front().iov_base)->id;
  for (auto& data : m_data_vect) {
    uint64_t id = pointer_cast<EventHeader>(data.front().iov_base)->id;
    uint64_t flags = pointer_cast<EventHeader>(data.front().iov_base)->flags;
    uint64_t length = pointer_cast<EventHeader>(data.front().iov_base)->length;
    if (id != local_evt_id) {
      LOG(ERROR)
        << "Remote event id ("
        << id
        << ") is different from local event id ("
        << local_evt_id
        << ")";
      return false;
    }
    if (flags >= m_endpoints.size()) {
      LOG(ERROR) << "Found wrong connection id: " << flags;
      return false;
    }
    if (!length) {
      LOG(ERROR) << "Found wrong length: " << length;
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
  if (id != m_id) {
    auto& conn = *(m_connection_ids.at(id));
    // Reset len of iovec
    for (auto& iov : sub_vect) {
      iov.iov_len = m_max_fragment_size * m_bulk_size;  // chunk size
    }
    conn.post_read(sub_vect);
  } else {
    for (auto& iov : sub_vect) {
      while (!m_free_local_queue.push(iov)) {
        ;
      }
    }
  }
  return bytes;
}

void BuilderUnit::operator()(std::shared_ptr<std::atomic<bool> > stop) {

  std::vector<int> id_sequence = create_sequence(m_id, m_endpoints.size());

  // Allocate memory

  size_t const data_size =
    m_max_fragment_size * m_bulk_size * m_credits * (m_endpoints.size() - 1);
  std::unique_ptr<unsigned char[]> const data_ptr(new unsigned char[data_size]);
  LOG(NOTICE) << "Builder Unit - Allocated " << data_size << " bytes of memory";

  // Connections

  Acceptor<RecvSocket> acceptor(m_credits);

  bool ep_created = false;
  while (!ep_created) {
    try {
      acceptor.listen(m_endpoints[m_id].hostname(), m_endpoints[m_id].port());
      ep_created = true;
    } catch (std::exception& e) {
      LOG(WARNING) << "Builder Unit - Error on listen ... Retrying!";
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }

  LOG(NOTICE) << "Builder Unit - Waiting for connections...";

  size_t const chunk_size = m_max_fragment_size * m_bulk_size;

  for (int i = 0; i < m_endpoints.size() - 1; ++i) {

    // Create a temporary entry in the map with the local id
    auto p = m_connection_ids.emplace(
      std::make_pair(m_id, std::unique_ptr<RecvSocket>(acceptor.accept())));
    int id = find_endpoint_id(m_endpoints, p.first->second->peer_hostname());
    assert(id != -1 && "Address not found in endpoints list.");
    // Insert the real id and delete the local one
    m_connection_ids.emplace(std::make_pair(id, std::move(p.first->second)));
    m_connection_ids.erase(p.first);
    auto& conn = *(m_connection_ids.at(id));

    unsigned char* base_data_ptr = data_ptr.get() + i * chunk_size * m_credits;
    conn.register_memory(base_data_ptr, chunk_size * m_credits);

    std::vector<iovec> iov_vect;
    for (int j = 0; j < m_credits; ++j) {
      iov_vect.push_back( { base_data_ptr + j * chunk_size, chunk_size });
    }
    conn.post_read(iov_vect);

    LOG(NOTICE)
      << "Builder Unit - Connection established with ip "
      << conn.peer_hostname();
  }
  LOG(NOTICE) << "Builder Unit - All connections established";

  FrequencyMeter frequency(5.0);
  FrequencyMeter bandwith(5.0);  // this timeout is ignored (frequency is used)

  std::chrono::high_resolution_clock::time_point t_tot =
    std::chrono::high_resolution_clock::now();
  std::chrono::high_resolution_clock::time_point t_active;
  double active_time = 0;

  while (!(*stop)) {

    bool active_flag = false;
    t_active = std::chrono::high_resolution_clock::now();

    // Acquire
    int min_wrs = m_credits;
    for (auto id : id_sequence) {
      int read_wrs = read_data(id);
      if (read_wrs) {
        LOG(DEBUG) << "Builder Unit - Read " << read_wrs << " wrs";
        active_flag = true;
      }
      int const current_wrs = m_data_vect[id].size();
      min_wrs = (min_wrs < current_wrs) ? min_wrs : current_wrs;
    }

    if (min_wrs) {
      if (!check_data()) {
        throw std::runtime_error("Error checking data");
      }

      // Release
      for (auto id : id_sequence) {
        size_t const bytes = release_data(id, min_wrs);
        if (id != m_endpoints.size() - 1) {
          bandwith.add(bytes);
        }
        LOG(DEBUG) << "Builder Unit - Released " << min_wrs << " wrs";
      }
      frequency.add(min_wrs * m_bulk_size * m_endpoints.size());
    }

    if (active_flag) {
      active_time += std::chrono::duration<double>(
        std::chrono::high_resolution_clock::now() - t_active).count();
    }

    if (frequency.check()) {

      double tot_time = std::chrono::duration<double>(
        std::chrono::high_resolution_clock::now() - t_tot).count();

      LOG(NOTICE)
        << "Builder Unit: "
        << frequency.frequency() / std::mega::num
        << " MHz - "
        << bandwith.frequency() / std::giga::num * 8.
        << " Gb/s - "
        << active_time / tot_time * 100.
        << " %";
      active_time = 0;
      t_tot = std::chrono::high_resolution_clock::now();
    }
  }

  LOG(INFO) << "Readout Unit: exiting";
}
}
