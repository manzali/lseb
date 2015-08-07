#include <memory>
#include <string>
#include <algorithm>
#include <map>

#include "common/frequency_meter.h"
#include "common/timer.h"
#include "common/configuration.h"
#include "common/log.hpp"
#include "common/dataformat.h"

#include "transport/endpoints.h"
#include "transport/transport.h"

#include "bu/builder_unit.h"

namespace lseb {

BuilderUnit::BuilderUnit(
  Configuration const& configuration,
  int id,
  boost::lockfree::spsc_queue<iovec>& free_local_data,
  boost::lockfree::spsc_queue<iovec>& ready_local_data)
    :
      m_configuration(configuration),
      m_free_local_data(free_local_data),
      m_ready_local_data(ready_local_data),
      m_id(id) {
}

void BuilderUnit::operator()() {

  int const max_fragment_size = m_configuration.get<int>(
    "GENERAL.MAX_FRAGMENT_SIZE");
  assert(max_fragment_size > 0);

  int const bulk_size = m_configuration.get<int>("GENERAL.BULKED_EVENTS");
  assert(bulk_size > 0);

  int const tokens = m_configuration.get<int>("GENERAL.TOKENS");
  assert(tokens > 0);

  std::vector<Endpoint> const endpoints = get_endpoints(
    m_configuration.get_child("ENDPOINTS"));

  size_t const data_size = max_fragment_size * bulk_size * tokens;

  // Allocate memory

  std::unique_ptr<unsigned char[]> const data_ptr(
    new unsigned char[data_size * (endpoints.size() - 1)]);

  LOG(NOTICE)
    << "Builder Unit - Allocated "
    << data_size * (endpoints.size() - 1)
    << " bytes of memory";

  // Connections

  BuSocket socket = lseb_listen(
    endpoints.at(m_id).hostname(),
    endpoints.at(m_id).port(),
    tokens);

  int next_id = m_id;
  std::vector<int> id_sequence(endpoints.size());
  for (auto& id : id_sequence) {
    next_id = (next_id != 0) ? next_id - 1 : endpoints.size() - 1;
    id = next_id;
  }

  LOG(NOTICE) << "Builder Unit - Waiting for connections...";
  std::map<int, BuConnectionId> connection_ids;
  for (int i = 0; i < endpoints.size() - 1; ++i) {
    BuConnectionId conn = lseb_accept(socket);
    lseb_register(conn, data_ptr.get() + i * data_size, data_size);
    auto p = connection_ids.emplace(conn.id, conn);
    assert(p.second && "connection id already present");
    LOG(NOTICE)
      << "Builder Unit - Connection established with "
      << lseb_get_peer_hostname(conn)
      << " (ru "
      << conn.id
      << ")";
  }
  LOG(NOTICE) << "Builder Unit - All connections established";

  FrequencyMeter frequency(5.0);
  FrequencyMeter bandwith(5.0);  // this timeout is ignored (frequency is used)

  Timer t_recv;
  Timer t_check;
  Timer t_rel;

  std::vector<std::vector<iovec> > data_vect(endpoints.size());

  while (true) {

    int min_wrs = tokens;

    // Acquire
    t_recv.start();
    do {
      min_wrs = tokens;
      for (auto id : id_sequence) {
        auto& iov_vect = data_vect.at(id);
        int old_size = iov_vect.size();
        if (id != m_id) {
          auto conn = connection_ids.find(id);
          assert(conn != std::end(connection_ids));
          std::vector<iovec> new_data = lseb_read(conn->second);
          if (!new_data.empty()) {
            iov_vect.insert(
              std::end(iov_vect),
              std::begin(new_data),
              std::end(new_data));
          }
        } else {
          iovec i;
          while (m_ready_local_data.pop(i)) {
            iov_vect.push_back(i);
          }
        }
        if (iov_vect.size() != old_size) {
          LOG(DEBUG)
            << "Read "
            << iov_vect.size() - old_size
            << " wr from conn "
            << id;
        }
        min_wrs = (min_wrs < iov_vect.size()) ? min_wrs : iov_vect.size();
      }
    } while (min_wrs == 0);
    t_recv.pause();

    // -----------------------------------------------------------------------
    // There should be event building here (for the first min_wrs multievents)
    // -----------------------------------------------------------------------
    t_check.start();
    uint64_t evt_id = pointer_cast<EventHeader>(
      data_vect.at(m_id).front().iov_base)->id;
    for (auto id : id_sequence) {
      uint64_t current_evt_id = pointer_cast<EventHeader>(
        data_vect.at(id).front().iov_base)->id;
      uint64_t current_flags = pointer_cast<EventHeader>(
        data_vect.at(id).front().iov_base)->flags;
      // LOG(NOTICE) << "id = " << id <<" - evt = " << current_evt_id << " - flags = " << current_flags;
      assert(evt_id == current_evt_id);
      assert(id == current_flags);
    }
    t_check.pause();

    // Release
    t_rel.start();
    for (auto id : id_sequence) {
      auto& iov_vect = data_vect.at(id);
      assert(iov_vect.size() >= min_wrs);
      std::vector<iovec> vect(
        std::begin(iov_vect),
        std::begin(iov_vect) + min_wrs);
      if (id != m_id) {
        bandwith.add(iovec_length(vect));
        auto conn = connection_ids.find(id);
        assert(conn != std::end(connection_ids));
        lseb_release(conn->second, vect);
      } else {
        for (auto& i : vect) {
          while (!m_free_local_data.push(i)) {
            ;
          }
        }
      }
      LOG(DEBUG) << "Released " << min_wrs << " wr for conn " << id;
      iov_vect.erase(std::begin(iov_vect), std::begin(iov_vect) + min_wrs);
    }
    t_rel.pause();
    frequency.add(min_wrs * bulk_size * endpoints.size());

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
