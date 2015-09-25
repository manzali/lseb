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

  // Allocate memory

  size_t const data_size = max_fragment_size * bulk_size * tokens * (endpoints
    .size() - 1);
  std::unique_ptr<unsigned char[]> const data_ptr(new unsigned char[data_size]);
  LOG(NOTICE) << "Builder Unit - Allocated " << data_size << " bytes of memory";

  // Connections

  Acceptor<RecvSocket> acceptor;
  acceptor.register_memory(data_ptr.get(), data_size, tokens);
  acceptor.listen(endpoints.at(m_id).hostname(), endpoints.at(m_id).port());

  LOG(NOTICE) << "Builder Unit - Waiting for connections...";

  size_t const chunk_size = max_fragment_size * bulk_size;

  std::vector<RecvSocket> connection_ids;
  for (int i = 0; i < endpoints.size() - 1; ++i) {
    connection_ids.push_back(acceptor.accept());
    RecvSocket& conn = connection_ids.back();

    std::vector<iovec> iov_vect;
    for (int j = 0; j < tokens; ++j) {
      iov_vect.push_back(
        { data_ptr.get() + i * chunk_size * tokens + j * chunk_size, chunk_size });
    }
    conn.release(iov_vect);

    LOG(NOTICE) << "Builder Unit - Connection established";
  }
  LOG(NOTICE) << "Builder Unit - All connections established";

  FrequencyMeter frequency(5.0);
  FrequencyMeter bandwith(5.0);  // this timeout is ignored (frequency is used)

  Timer t_recv;
  Timer t_check;
  Timer t_rel;

  std::vector<std::vector<iovec> > data_vect(endpoints.size());
  // The local data is in the last position of data_vect

  while (true) {

    int min_wrs = tokens;

    // Acquire
    t_recv.start();
    do {
      min_wrs = tokens;

      for (int i = 0; i < endpoints.size(); ++i) {

        auto& iov_vect = data_vect[i];
        int old_size = iov_vect.size();

        if (i != endpoints.size() - 1) {
          auto& conn = connection_ids[i];
          std::vector<iovec> new_data = conn.read_all();
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
          LOG(DEBUG) << "Read " << iov_vect.size() - old_size << " wr";
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
      data_vect[m_id].front().iov_base)->id;
    for (auto& data : data_vect) {
      uint64_t current = pointer_cast<EventHeader>(data.front().iov_base)->id;
      assert(evt_id == current);
    }
    t_check.pause();

    // Release
    t_rel.start();

    for (int i = 0; i < endpoints.size(); ++i) {

      auto& iov_vect = data_vect[i];
      assert(iov_vect.size() >= min_wrs);
      std::vector<iovec> sub_vect(
        std::begin(iov_vect),
        std::begin(iov_vect) + min_wrs);

      if (i != endpoints.size() - 1) {
        bandwith.add(iovec_length(sub_vect));
        auto& conn = connection_ids[i];
        // Reset len of iovec
        for(auto& iov : sub_vect){
          iov.iov_len = chunk_size;
        }
        conn.release(sub_vect);
      } else {
        for (auto& i : sub_vect) {
          while (!m_free_local_data.push(i)) {
            ;
          }
        }
      }
      LOG(DEBUG) << "Released " << min_wrs << " wr";
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
