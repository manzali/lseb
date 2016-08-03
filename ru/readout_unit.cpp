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
    int bulk_size,
    int credits,
    int id)
    : m_accumulator(accumulator),
      m_bulk_size(bulk_size),
      m_credits(credits),
      m_id(id) {
}

void ReadoutUnit::connect(std::vector<Endpoint> const& endpoints){
  std::vector<int> id_sequence = create_sequence(m_id, endpoints.size());

  LOG(NOTICE) << "Readout Unit - Waiting for connections...";

  DataRange const data_range = m_accumulator.data_range();
  Connector connector(m_credits);

  for (auto id : id_sequence) {
    Endpoint const& ep = endpoints[id];
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
  LOG(NOTICE) << "Readout Unit - All connections established";
}

void ReadoutUnit::run() {

  std::vector<int> id_sequence = create_sequence(m_id, m_connection_ids.size());

  FrequencyMeter bandwith(5.0);

  std::chrono::high_resolution_clock::time_point t_tot =
      std::chrono::high_resolution_clock::now();
  std::chrono::high_resolution_clock::time_point t_start;
  //double active_time = 0;

  auto seq_it = std::begin(id_sequence);
  std::vector<iovec> iov_to_send;

  while (true) {

    t_start = std::chrono::high_resolution_clock::now();
    //bool active_flag = false;
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
      auto& conn = *(m_connection_ids.at(id));
      std::vector<iovec> completed_wr = conn.poll_completed_send();
      for (auto const& wr : completed_wr) {
        if (id != m_id) {
          bandwith.add(wr.iov_len);
        }
        wr_to_release.push_back(wr.iov_base);
      }
      int const count = completed_wr.size();
      conn_avail = (seq_id == id) ? (conn.available_send()) : conn_avail;
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
      //active_flag = true;
      m_accumulator.release_multievents(wr_to_release);
    }

    // If there are free resources for this connection and ready data, send it
    if (conn_avail && iov_to_send.size() > seq_id) {
      //active_flag = true;
      auto& iov = iov_to_send[seq_id];
      auto& conn = *(m_connection_ids.at(seq_id));
      conn.post_send(iov);
      LOG(DEBUG) << "Readout Unit - Written 1 wrs to conn " << seq_id;

      // Increment seq_it and check for end of a cycle
      if (++seq_it == std::end(id_sequence)) {
        // End of a cycle
        seq_it = std::begin(id_sequence);
        assert(iov_to_send.size() == m_connection_ids.size());
        iov_to_send.clear();
      }
    }

    //if (active_flag) {
    //  active_time += std::chrono::duration<double>(
    //      std::chrono::high_resolution_clock::now() - t_start).count();
    //}

    if (bandwith.check()) {

      double tot_time = std::chrono::duration<double>(
          std::chrono::high_resolution_clock::now() - t_tot).count();

      LOG(NOTICE)
        << "Readout Unit: "
        << bandwith.frequency() / std::giga::num * 8.
        << " Gb/s";
      //  << active_time / tot_time * 100.
      //  << " %";
      //active_time = 0;
      t_tot = std::chrono::high_resolution_clock::now();
    }
  }

  LOG(INFO) << "Readout Unit: exiting";
}

}
