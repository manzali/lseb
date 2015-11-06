#include <memory>
#include <string>
#include <map>
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
#include "common/timer.h"

namespace lseb {

std::vector<int> create_sequence(int id, int total) {
  int first_id = (id != total - 1) ? id + 1 : 0;
  std::vector<int> sequence(total);
  std::iota(std::begin(sequence), std::end(sequence), 0);
  std::rotate(
    std::begin(sequence),
    std::begin(sequence) + first_id,
    std::end(sequence));
  return sequence;
}

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

void ReadoutUnit::operator()() {

  int const required_multievents = m_endpoints.size();
  std::vector<int> id_sequence = create_sequence(m_id, required_multievents);

  LOG(NOTICE) << "Readout Unit - Waiting for connections...";

  DataRange const data_range = m_accumulator.data_range();
  Connector<SendSocket> connector(m_credits);

  for (auto id : id_sequence) {
    if (id != m_id) {
      Endpoint const& ep = m_endpoints.at(id);
      bool connected = false;
      while (!connected) {
        try {
          auto ret = m_connection_ids.emplace(
            std::make_pair(id, connector.connect(ep.hostname(), ep.port())));
          assert(ret.second && "Connection already present");
          ret.first->second.register_memory(
            (void*) std::begin(data_range),
            std::distance(std::begin(data_range), std::end(data_range)));
          connected = true;
        } catch (std::exception& e) {
          std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
      }
      LOG(NOTICE)
        << "Readout Unit - Connection established with ip "
        << m_connection_ids.find(id)->second.peer_hostname()
        << " (bu"
        << id
        << ")";
    }
  }
  LOG(NOTICE) << "Readout Unit - All connections established";

  FrequencyMeter frequency(5.0);
  FrequencyMeter bandwith(5.0);  // this timeout is ignored (frequency is used)

  Timer t_ctrl;
  Timer t_send;
  Timer t_release;

  while (true) {

    // Release
    t_release.start();
    std::vector<iovec> iov_to_release;
    for (auto id : id_sequence) {
      bool available = false;
      do {
        std::vector<iovec> completed_iov;
        if (id != m_id) {
          auto conn_it = m_connection_ids.find(id);
          assert(conn_it != std::end(m_connection_ids));
          auto& conn = conn_it->second;
          completed_iov = conn.pop_completed();
          available = (conn.pending() != m_credits);
        } else {
          iovec iov;
          while (m_free_local_queue.pop(iov)) {
            completed_iov.push_back(iov);
            --m_pending_local_iov;
            assert(
              m_pending_local_iov >= 0 && m_pending_local_iov <= m_credits);
          }
          available = (m_pending_local_iov != m_credits);
        }
        if (!completed_iov.empty()) {
          iov_to_release.insert(
            std::end(iov_to_release),
            std::begin(completed_iov),
            std::end(completed_iov));
          LOG(DEBUG)
            << "Readout Unit - Completed "
            << completed_iov.size()
            << " iov to conn "
            << id;
        }
      } while (!available);
    }
    if (!iov_to_release.empty()) {
      m_accumulator.release_multievents(iov_to_release);
    }
    t_release.pause();

    // Acquire
    t_ctrl.start();
    std::vector<iovec> iov_to_send = m_accumulator.get_multievents();
    t_ctrl.pause();

    // Send
    if (!iov_to_send.empty()) {
      assert(iov_to_send.size() == required_multievents);
      t_send.start();
      for (auto id : id_sequence) {
        auto& iov = iov_to_send.at(id);
        if (id != m_id) {
          auto conn_it = m_connection_ids.find(id);
          assert(conn_it != std::end(m_connection_ids));
          auto& conn = conn_it->second;
          assert(m_credits - conn.pending() != 0);
          bandwith.add(conn.post_write(iov));
        } else {
          while (!m_ready_local_queue.push(iov)) {
            ;
          }
          ++m_pending_local_iov;
          assert(m_pending_local_iov >= 0 && m_pending_local_iov <= m_credits);
        }
        LOG(DEBUG) << "Readout Unit - Writing iov to conn " << id;
      }
      t_send.pause();
      frequency.add(required_multievents * m_bulk_size);
    }

    if (frequency.check()) {
      LOG(NOTICE)
        << "Readout Unit: "
        << frequency.frequency() / std::mega::num
        << " MHz - "
        << bandwith.frequency() / std::giga::num * 8.
        << " Gb/s";

      LOG(INFO)
        << "Times:\n"
        << "\tt_ctrl: "
        << t_ctrl.rate()
        << "%\n"
        << "\tt_release: "
        << t_release.rate()
        << "%\n"
        << "\tt_send: "
        << t_send.rate()
        << "%";
      t_ctrl.reset();
      t_release.reset();
      t_send.reset();
    }
  }
}

}
