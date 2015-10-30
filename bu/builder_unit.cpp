#include <memory>
#include <string>
#include <algorithm>
#include <map>

#include "common/frequency_meter.h"
#include "common/timer.h"
#include "common/log.hpp"
#include "common/dataformat.h"

#include "bu/builder_unit.h"

namespace lseb {

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
  if (id != m_endpoints.size() - 1) {
    auto& conn = m_connection_ids[id];
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

size_t BuilderUnit::release_data(int id, int n){
  auto& iov_vect = m_data_vect[id];
  assert(iov_vect.size() >= n);
  std::vector<iovec> sub_vect(
    std::begin(iov_vect),
    std::begin(iov_vect) + n);
  // Erase iovec
  iov_vect.erase(std::begin(iov_vect), std::begin(iov_vect) + n);
  size_t const bytes = iovec_length(sub_vect);
  // Release iovec
  if (id != m_endpoints.size() - 1) {
    auto& conn = m_connection_ids[id];
    // Reset len of iovec
    for (auto& iov : sub_vect) {
      iov.iov_len = m_max_fragment_size * m_bulk_size; // chunk size
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

void BuilderUnit::operator()() {

  // Allocate memory

  size_t const data_size =
    m_max_fragment_size * m_bulk_size * m_credits * (m_endpoints.size() - 1);
  std::unique_ptr<unsigned char[]> const data_ptr(new unsigned char[data_size]);
  LOG(NOTICE) << "Builder Unit - Allocated " << data_size << " bytes of memory";

  // Connections

  Acceptor<RecvSocket> acceptor(m_credits);
  acceptor.listen(m_endpoints.at(m_id).hostname(), m_endpoints.at(m_id).port());

  LOG(NOTICE) << "Builder Unit - Waiting for connections...";

  size_t const chunk_size = m_max_fragment_size * m_bulk_size;

  for (int i = 0; i < m_endpoints.size() - 1; ++i) {
    m_connection_ids.push_back(acceptor.accept());
    RecvSocket& conn = m_connection_ids.back();

    unsigned char* base_data_ptr = data_ptr.get() + i * chunk_size * m_credits;
    conn.register_memory(base_data_ptr, chunk_size * m_credits);

    std::vector<iovec> iov_vect;
    for (int j = 0; j < m_credits; ++j) {
      iov_vect.push_back( { base_data_ptr + j * chunk_size, chunk_size });
    }
    conn.post_read(iov_vect);

    LOG(NOTICE) << "Builder Unit - Connection established";
  }
  LOG(NOTICE) << "Builder Unit - All connections established";

  FrequencyMeter frequency(5.0);
  FrequencyMeter bandwith(5.0);  // this timeout is ignored (frequency is used)

  Timer t_recv;
  Timer t_check;
  Timer t_rel;

  // The local data is in the last position of m_data_vect

  while (true) {

    // Acquire
    t_recv.start();
    int min_wrs = m_credits;
    do {
      min_wrs = m_credits;
      for (int i = 0; i < m_endpoints.size(); ++i) {
        int read_wrs = read_data(i);
        if (read_wrs) {
          LOG(DEBUG) << "Builder Unit - Read " << read_wrs << " wrs";
        }
        int const xxx = m_data_vect[i].size();
        min_wrs = (min_wrs < xxx) ? min_wrs : xxx;
      }
    } while (min_wrs == 0);
    t_recv.pause();

    t_check.start();
    if (!check_data()) {
      throw std::runtime_error("Error checking data");
    }
    t_check.pause();

    // Release
    t_rel.start();
    for (int i = 0; i < m_endpoints.size(); ++i) {
      size_t const bytes = release_data(i, min_wrs);
      if (i != m_endpoints.size() - 1) {
        bandwith.add(bytes);
      }
      LOG(DEBUG) << "Builder Unit - Released " << min_wrs << " wrs";
    }
    t_rel.pause();
    frequency.add(min_wrs * m_bulk_size * m_endpoints.size());

    if (frequency.check()) {
      LOG(NOTICE)
        << "Builder Unit: "
        << frequency.frequency() / std::mega::num
        << " MHz - "
        << bandwith.frequency() / std::giga::num * 8.
        << " Gb/s";

      LOG(INFO)
        << "Times:\n"
        << "\tt_recv: "
        << t_recv.rate()
        << "%\n"
        << "\tt_check: "
        << t_check.rate()
        << "%\n"
        << "\tt_rel: "
        << t_rel.rate()
        << "%";
      t_recv.reset();
      t_check.reset();
      t_rel.reset();
    }
  }
}
}
