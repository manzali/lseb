#include <memory>
#include <string>
#include <cstdlib>
#include <cassert>
#include <algorithm>

#include "common/dataformat.h"
#include "common/log.hpp"
#include "common/utility.h"
#include "common/configuration.h"
#include "common/frequency_meter.h"
#include "common/timer.h"

#include "generator/generator.h"
#include "generator/length_generator.h"

#include "ru/controller.h"
#include "ru/accumulator.h"
//#include "ru/sender.h"
#include "ru/splitter.h"
#include "ru/readout_unit.h"

#include "transport/transport.h"
#include "transport/endpoints.h"

namespace lseb {

ReadoutUnit::ReadoutUnit(
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

void ReadoutUnit::operator()() {

  int const generator_frequency = m_configuration.get<int>(
    "GENERATOR.FREQUENCY");
  assert(generator_frequency > 0);

  int const mean = m_configuration.get<int>("GENERATOR.MEAN");
  assert(mean > 0);

  int const stddev = m_configuration.get<int>("GENERATOR.STD_DEV");
  assert(stddev > 0);

  int const max_fragment_size = m_configuration.get<int>(
    "GENERAL.MAX_FRAGMENT_SIZE");
  assert(max_fragment_size > 0);

  int const bulk_size = m_configuration.get<int>("GENERAL.BULKED_EVENTS");
  assert(bulk_size > 0);

  int const tokens = m_configuration.get<int>("GENERAL.TOKENS");
  assert(tokens > 0);

  int const meta_size = m_configuration.get<int>("RU.META_BUFFER");
  assert(meta_size > 0);

  int const data_size = m_configuration.get<int>("RU.DATA_BUFFER");
  assert(data_size > 0);

  std::chrono::milliseconds ms_timeout(
    m_configuration.get<int>("RU.MS_TIMEOUT"));

  std::vector<Endpoint> const endpoints = get_endpoints(
    m_configuration.get_child("ENDPOINTS"));

  assert(
    meta_size % sizeof(EventMetaData) == 0 && "wrong metadata buffer size");

  LOG(INFO) << "Waiting for connections...";
  std::vector<RuConnectionId> connection_ids;
  std::transform(
    std::begin(endpoints),
    std::end(endpoints),
    std::back_inserter(connection_ids),
    [tokens](Endpoint const& endpoint) {
      return lseb_connect(endpoint.hostname(), endpoint.port(), tokens);
    });
  LOG(INFO) << "Connections established";

  // Allocate memory

  std::unique_ptr<unsigned char[]> const metadata_ptr(
    new unsigned char[meta_size]);
  std::unique_ptr<unsigned char[]> const data_ptr(new unsigned char[data_size]);

  MetaDataRange metadata_range(
    pointer_cast<EventMetaData>(metadata_ptr.get()),
    pointer_cast<EventMetaData>(metadata_ptr.get() + meta_size));
  DataRange data_range(data_ptr.get(), data_ptr.get() + data_size);

  MetaDataBuffer metadata_buffer(
    std::begin(metadata_range),
    std::end(metadata_range));
  DataBuffer data_buffer(std::begin(data_range), std::end(data_range));

  Accumulator accumulator(metadata_range, data_range, bulk_size);

  for (auto& conn : connection_ids) {
    lseb_register(conn, data_ptr.get(), data_size);
  }

  //Sender sender(connection_ids);

  LengthGenerator payload_size_generator(mean, stddev, max_fragment_size);
  Generator generator(
    payload_size_generator,
    metadata_buffer,
    data_buffer,
    m_id);
  Controller controller(generator, metadata_range, generator_frequency);

  Splitter splitter(m_id, connection_ids.size(), data_range);

  FrequencyMeter frequency(1.0);
  FrequencyMeter bandwith(1.0);

  Timer t_accu;
  Timer t_send;
  Timer t_spli;

  unsigned int const needed_events = bulk_size * tokens * endpoints.size();
  unsigned int const needed_multievents = tokens * endpoints.size();

  while (true) {

    t_accu.start();
    unsigned int ready_events = 0;
    do {
      ready_events = accumulator.add(controller.read());
    } while (needed_events >= ready_events);
    MultiEvents multievents = accumulator.get_multievents(needed_multievents);
    t_accu.pause();

    assert(multievents.size() == needed_multievents);

    t_spli.start();
    std::map<int, std::vector<DataIov> > iov_map = splitter.split(multievents);
    t_spli.pause();

    t_send.start();
    for (auto& iov_pair : iov_map) {
      assert(iov_pair.second.size() == tokens);
      while (lseb_avail(connection_ids[iov_pair.first]) < iov_pair.second.size()) {
        ;
      }
      bandwith.add(lseb_write(connection_ids[iov_pair.first], iov_pair.second));
    }
    //size_t sent_bytes = sender.send(iov_map, ms_timeout);
    t_send.pause();
    //bandwith.add(sent_bytes);

    //m_free_local_data.pop();
    //m_ready_local_data.push(iovec { });

    t_accu.start();
    controller.release(
      MetaDataRange(
        std::begin(multievents.front().first),
        std::end(multievents.back().first)));
    t_accu.pause();

    frequency.add(multievents.size() * bulk_size);

    if (frequency.check()) {
      LOG(INFO)
        << "Frequency: "
        << frequency.frequency() / std::mega::num
        << " MHz";
    }

    if (bandwith.check()) {
      LOG(INFO)
        << "Bandwith: "
        << bandwith.frequency() / std::giga::num * 8.
        << " Gb/s";

      LOG(INFO)
        << "Times:\n"
        << "\tt_accu: "
        << t_accu.rate()
        << "%\n"
        << "\tt_send: "
        << t_send.rate()
        << "%\n"
        << "\tt_spli: "
        << t_spli.rate()
        << "%";
      t_accu.reset();
      t_send.reset();
      t_spli.reset();
    }
  }
}

}
