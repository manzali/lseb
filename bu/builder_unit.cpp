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

  LOG(INFO)
    << "Allocated "
    << data_size * (endpoints.size() - 1)
    << " bytes of memory";

  // Connections

  BuSocket socket = lseb_listen(
    endpoints[m_id].hostname(),
    endpoints[m_id].port(),
    tokens);

  LOG(NOTICE) << "Waiting for connections...";
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
  LOG(NOTICE) << "Connections established";

  FrequencyMeter frequency(5.0);
  FrequencyMeter bandwith(5.0);

  Timer t_recv;
  Timer t_rel;

  std::map<int, std::vector<iovec> > iov_map;
  for (int i = 0; i < endpoints.size(); ++i) {
    iov_map.insert(std::make_pair(i, std::vector<iovec>()));
  }

  std::vector<int> diff(endpoints.size(), 0);

  while (true) {

    // Acquire
    t_recv.start();
    for (auto&m : iov_map) {
      int old_size = m.second.size();
      if (m.first != m_id) {
        auto conn = connection_ids.find(m.first);
        assert(conn != std::end(connection_ids));
        std::vector<iovec> vect = lseb_read(conn->second);
        if (!vect.empty()) {
          m.second.insert(std::end(m.second), std::begin(vect), std::end(vect));
        }
      } else {
        iovec i;
        while (m_ready_local_data.pop(i)) {
          m.second.push_back(i);
        }
      }
      if (m.second.size() != old_size) {
        LOG(DEBUG)
          << "Read "
          << m.second.size() - old_size
          << " wr from conn "
          << m.first;
        diff[m.first] += m.second.size() - old_size;
      }
    }
    t_recv.pause();

    // -----------------------------------------------------------------------
    // There should be event building here (for the first min_wrs multievents)
    // -----------------------------------------------------------------------

    // Release
    t_rel.start();
    for (auto& m : iov_map) {
      if (!m.second.empty()) {
        bandwith.add(iovec_length(m.second));
        frequency.add(m.second.size() * bulk_size);
        if (m.first != m_id) {
          auto conn = connection_ids.find(m.first);
          assert(conn != std::end(connection_ids));
          lseb_release(conn->second, m.second);
        } else {
          for (auto& i : m.second) {
            while (!m_free_local_data.push(i)) {
              ;
            }
          }
        }
        LOG(DEBUG)
          << "Released "
          << m.second.size()
          << " wr for conn "
          << m.first;
        m.second.clear();
      }
    }
    t_rel.pause();

    int min = std::distance(
      std::begin(diff),
      std::min_element(std::begin(diff), std::end(diff)));
    int val = diff[min];
    for (auto& i : diff) {
      i -= val;
    }

    if (bandwith.check()) {
      LOG(NOTICE)
        << "Builder Unit - Bandwith: "
        << bandwith.frequency() / std::giga::num * 8.
        << " Gb/s";
    }

    if (frequency.check()) {
      LOG(NOTICE)
        << "Builder Unit - Frequency: "
        << frequency.frequency() / std::mega::num
        << " MHz";

      int count = 0;
      for (auto i : diff) {
        LOG(NOTICE) << "Diff " << count << ": " << i << std::endl;
        ++count;
      }

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
