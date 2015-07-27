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
  SharedQueue<iovec>& free_local_data,
  SharedQueue<iovec>& ready_local_data)
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

  std::chrono::milliseconds ms_timeout(
    m_configuration.get<int>("BU.MS_TIMEOUT"));

  size_t const data_size = max_fragment_size * bulk_size * tokens;

  // Allocate memory

  std::unique_ptr<unsigned char[]> const data_ptr(
    new unsigned char[data_size * (endpoints.size() - 1)]);

  LOG(INFO)
    << "Allocated "
    << data_size * (endpoints.size() - 1)
    << " bytes of memory";

  // Connections

  BuSocket socket = lseb_listen(
    endpoints[m_id].hostname(),
    endpoints[m_id].port(),
    tokens);

  LOG(INFO) << "Waiting for connections...";
  std::map<int, BuConnectionId> connection_ids;
  int count = 0;
  for (int i = 0; i < endpoints.size(); ++i) {
    if (i != m_id) {
      auto p = connection_ids.emplace(i, lseb_accept(socket));
      lseb_register(
        p.first->second,
        data_ptr.get() + count++ * data_size,
        data_size);
    }
  }
  LOG(INFO) << "Connections established";

  FrequencyMeter bandwith(1.0);
  Timer t_recv;
  Timer t_rel;

  std::map<int, std::vector<iovec> > iov_map;
  for (int i = 0; i < endpoints.size(); ++i) {
    iov_map.insert(std::make_pair(i, std::vector<iovec>()));
  }

  while (true) {

    t_recv.start();
    int min_wrs;
    do {
      min_wrs = tokens;
      for (auto& m : iov_map) {
        if (m.first != m_id) {
          auto conn = connection_ids.find(m.first);
          assert(conn != std::end(connection_ids));
          std::vector<iovec> vect = lseb_read(conn->second);
          if (!vect.empty()) {
            LOG(DEBUG) << "Read " << vect.size() << " wr from conn " << m.first;
            m.second.insert(
              std::end(m.second),
              std::begin(vect),
              std::end(vect));
          }
        } else {
          iovec i;
          if (m_ready_local_data.pop_nowait(i)) {
            LOG(DEBUG) << "Read " << 1 << " wr from conn " << m_id;
            m.second.push_back(i);
          }
        }
        min_wrs = (min_wrs < m.second.size()) ? min_wrs : m.second.size();
      }
    } while (!min_wrs);
    t_recv.pause();

    // check

    t_rel.start();
    for (auto& m : iov_map) {
      std::vector<iovec> vect(
        std::begin(m.second),
        std::begin(m.second) + min_wrs);
      bandwith.add(iovec_length(vect));
      if (m.first != m_id) {
        auto conn = connection_ids.find(m.first);
        assert(conn != std::end(connection_ids));
        lseb_release(conn->second, vect);
        LOG(DEBUG)
          << "Released "
          << vect.size()
          << " wr for conn "
          << conn->first;
      } else {
        for (auto& i : vect) {
          m_free_local_data.push(i);
        }
      }
      m.second.erase(std::begin(m.second) + min_wrs);
    }
    t_rel.pause();

    if (bandwith.check()) {
      LOG(INFO)
        << "Bandwith: "
        << bandwith.frequency() / std::giga::num * 8.
        << " Gb/s";

      LOG(INFO)
        << "Times:\n"
        << "\tt_recv: "
        << t_recv.rate()
        << "%\n"
        << "\tt_rel: "
        << t_rel.rate()
        << "%";
      t_recv.reset();
      t_rel.reset();
    }
  }
}
}
