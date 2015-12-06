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

    bool active_flag = false;
    t_start = std::chrono::high_resolution_clock::now();

    int id = *seq_it;
    bool conn_avail = false;

    // Check for data to acquire
    std::pair<iovec, bool> p;
    p.second = true;
    int old_size = iov_to_send.size();
    for (int i = old_size; i <= id && p.second; ++i) {
      p = m_accumulator.get_multievent();
      if (p.second) {
        iov_to_send.push_back(p.first);
      }
    }
    if (iov_to_send.size() != old_size) {
      active_flag = true;
      LOG(DEBUG)
        << "Readout Unit - Acquired "
        << iov_to_send.size() - old_size
        << " multievents";
    }

    // Check for completed wr
    std::vector<void*> completed_wr;
    if (id != m_id) {
      auto& conn = *(m_connection_ids.at(id));
      completed_wr = conn.pop_completed();
      conn_avail = (conn.pending() != m_credits);
    } else {
      iovec iov;
      while (m_free_local_queue.pop(iov)) {
        completed_wr.push_back(iov.iov_base);
        --m_pending_local_iov;
        assert(m_pending_local_iov >= 0 && m_pending_local_iov <= m_credits);
      }
      conn_avail = (m_pending_local_iov != m_credits);
    }

    // Release completed wr
    if (!completed_wr.empty()) {
      active_flag = true;
      LOG(DEBUG)
        << "Readout Unit - Completed "
        << completed_wr.size()
        << " iov to conn "
        << id;
      m_accumulator.release_multievents(completed_wr);
    }

    // If there are free resources for this connection and ready data, send it
    if (conn_avail && iov_to_send.size() > id) {
      active_flag = true;
      auto& iov = iov_to_send[id];
      if (id != m_id) {
        auto& conn = *(m_connection_ids.at(id));
        assert(m_credits - conn.pending() != 0);
        bandwith.add(conn.post_write(iov));
      } else {
        while (!m_ready_local_queue.push(iov)) {
          ;
        }
        ++m_pending_local_iov;
        assert(
          m_pending_local_iov >= 0 && m_pending_local_iov <= m_credits);
      }
      LOG(DEBUG) << "Readout Unit - Writing iov to conn " << id;
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
