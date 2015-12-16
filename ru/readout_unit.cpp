#include <memory>
#include <string>
#include <thread>
#include <chrono>
#include <algorithm>

#include <cstdlib>
#include <cassert>

#include "ru/readout_unit.h"

#include "common/dataformat.h"
#include "common/log.hpp"
#include "common/utility.h"
#include "common/frequency_meter.h"

namespace lseb {

ReadoutUnit::ReadoutUnit(
  Accumulator& accumulator,
  boost::lockfree::spsc_queue<iovec>& free_local_data,
  boost::lockfree::spsc_queue<iovec>& ready_local_data,
  std::vector<Endpoint> const& endpoints,
  int bulk_size,
  int credits,
  int id)
    :
      m_accumulator(accumulator),
      m_free_local_queue(free_local_data),
      m_ready_local_queue(ready_local_data),
      m_endpoints(endpoints),
      m_bulk_size(bulk_size),
      m_credits(credits),
      m_id(id),
      m_pending_local_iov(0) {
}

void ReadoutUnit::operator()(std::shared_ptr<std::atomic<bool> > stop) {

  std::vector<int> id_sequence = create_sequence(m_id, m_endpoints.size());

  LOG(NOTICE) << "Readout Unit - Waiting for connections...";

  DataRange const data_range = m_accumulator.data_range();
  Connector<SendSocket> connector(m_credits);

  for (auto id : id_sequence) {
    if (id != m_id) {
      Endpoint const& ep = m_endpoints[id];
      bool connected = false;
      while (!connected) {
        try {
          auto ret = m_connection_ids.emplace(
            std::make_pair(id, connector.connect(ep.hostname(), ep.port())));
          assert(ret.second && "Connection already present");
          ret.first->second->register_memory(
            (void*) std::begin(data_range),
            std::distance(std::begin(data_range), std::end(data_range)));
          connected = true;
        } catch (std::exception& e) {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
      }
      LOG(NOTICE)
        << "Readout Unit - Connection established with ip "
        << ep.hostname()
        << " (bu "
        << id
        << ")";
    }
  }
  LOG(NOTICE) << "Readout Unit - All connections established";

  FrequencyMeter frequency(5.0);
  FrequencyMeter bandwith(5.0);  // this timeout is ignored (frequency is used)

  std::chrono::high_resolution_clock::time_point t_tot =
    std::chrono::high_resolution_clock::now();
  std::chrono::high_resolution_clock::time_point t_start;
  double active_time = 0;

  auto seq_it = std::begin(id_sequence);
  std::vector<iovec> iov_to_send;

  while (!(*stop)) {

    t_start = std::chrono::high_resolution_clock::now();
    bool active_flag = false;
    bool conn_avail = false;
    int seq_id = *seq_it;

    // Check for data to acquire
    std::pair<iovec, bool> p;
    p.second = true;
    for (int i = iov_to_send.size(); i <= seq_id && p.second; ++i) {
      p = m_accumulator.get_multievent();
      if (p.second) {
        iov_to_send.push_back(p.first);
      }
    }

    // Check for completed wr (in all connections)
    std::vector<void*> wr_to_release;
    for (auto id : id_sequence) {
      int count = 0;
      if (id != m_id) {
        auto& conn = *(m_connection_ids.at(id));
        std::vector<iovec> completed_wr = conn.pop_completed();
        for (auto const& wr : completed_wr) {
          bandwith.add(wr.iov_len);
          wr_to_release.push_back(wr.iov_base);
        }
        count = completed_wr.size();
        conn_avail = (seq_id == id) ? (conn.pending() != m_credits) : conn_avail;
      } else {
        iovec iov;
        while (m_free_local_queue.pop(iov)) {
          wr_to_release.push_back(iov.iov_base);
          ++count;
          --m_pending_local_iov;
          assert(m_pending_local_iov >= 0 && m_pending_local_iov <= m_credits);
        }
        conn_avail = (seq_id == id) ? (m_pending_local_iov != m_credits) : conn_avail;
      }
      if (!count) {
         LOG(DEBUG)
           << "Readout Unit - Completed "
           << count
           << " wrs of conn "
           << id;
       }
    }

    // Release completed wr
    if (!wr_to_release.empty()) {
      active_flag = true;
      m_accumulator.release_multievents(wr_to_release);
    }

    // If there are free resources for this connection and ready data, send it
    if (conn_avail && iov_to_send.size() > seq_id) {
      active_flag = true;
      auto& iov = iov_to_send[seq_id];
      if (seq_id != m_id) {
        auto& conn = *(m_connection_ids.at(seq_id));
        assert(m_credits - conn.pending() != 0);
        conn.post_send(iov);
      } else {
        while (!m_ready_local_queue.push(iov)) {
          ;
        }
        ++m_pending_local_iov;
        assert(
          m_pending_local_iov >= 0 && m_pending_local_iov <= m_credits);
      }
      LOG(DEBUG) << "Readout Unit - Written 1 wrs to conn " << seq_id;
      frequency.add(m_bulk_size);

      // Increment seq_it and check for end of a cycle
      if (++seq_it == std::end(id_sequence)) {
        // End of a cycle
        seq_it = std::begin(id_sequence);
        assert(iov_to_send.size() == m_endpoints.size());
        iov_to_send.clear();
      }
    }

    if(active_flag){
      active_time += std::chrono::duration<double>(
        std::chrono::high_resolution_clock::now() - t_start).count();
    }

    if (frequency.check()) {

      double tot_time = std::chrono::duration<double>(
        std::chrono::high_resolution_clock::now() - t_tot).count();

      LOG(NOTICE)
        << "Readout Unit: "
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
