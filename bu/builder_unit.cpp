#include <memory>
#include <string>
#include <algorithm>

#include "common/frequency_meter.h"
#include "common/timer.h"
#include "common/configuration.h"
#include "common/log.hpp"
#include "common/dataformat.h"

#include "transport/endpoints.h"
#include "transport/transport.h"

#include "bu/receiver.h"
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
    new unsigned char[data_size * endpoints.size()]);

  LOG(INFO)
    << "Allocated "
    << data_size * endpoints.size()
    << " bytes of memory";

  // Connections

  BuSocket socket = lseb_listen(
    endpoints[m_id].hostname(),
    endpoints[m_id].port(),
    tokens);

  std::vector<BuConnectionId> connection_ids;

  LOG(INFO) << "Waiting for connections...";
  int endpoint_count = 0;
  std::transform(
    std::begin(endpoints),
    std::end(endpoints),
    std::back_inserter(connection_ids),
    [&](Endpoint const& endpoint) {
      BuConnectionId conn = lseb_accept(socket);
      lseb_register(conn, data_ptr.get() + endpoint_count++ * data_size,data_size);
      return conn;
    });
  LOG(INFO) << "Connections established";

  Receiver receiver(connection_ids);

  FrequencyMeter bandwith(1.0);
  Timer t_recv;
  Timer t_rel;

  while (true) {

    t_recv.start();
    std::map<int, std::vector<iovec> > iov_map;
    for (int i = 0; i < connection_ids.size(); ++i) {
      std::vector<iovec> conn_iov;
      while (conn_iov.size() < tokens) {
        std::vector<iovec> temp = lseb_read(connection_ids[i]);
        conn_iov.insert(std::end(conn_iov), std::begin(temp), std::end(temp));
      }
      assert(conn_iov.size() == tokens);
      iov_map.emplace(i, conn_iov);
    }
    //std::map<int, std::vector<iovec> > iov_map = receiver.receive();

    t_recv.pause();

    //m_ready_local_data.pop();

    t_rel.start();
    if (!iov_map.empty()) {
      for (auto const& iov_pair : iov_map) {
        bandwith.add(iovec_length(iov_pair.second));
      }
      receiver.release(iov_map);
    }
    t_rel.pause();

    //m_free_local_data.push(iovec { });

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
