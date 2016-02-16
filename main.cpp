#include <thread>
#include <iostream>
#include <fstream>

#include <boost/asio.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/program_options.hpp>

#include <cassert>

#include "bu/builder_unit.h"
#include "ru/readout_unit.h"

#include "common/log.hpp"
#include "common/configuration.h"
#include "common/dataformat.h"
#include "common/tcp_barrier.h"

#include "transport/endpoints.h"

using namespace lseb;

int main(int argc, char* argv[]) {

  std::string str_conf;
  std::string str_logdir;
  std::string str_nodename;
  int timeout = 0;
  boost::program_options::options_description desc("Options");

  desc.add_options()("help,h", "Print help messages.")(
      "configuration,c",
      boost::program_options::value<std::string>(&str_conf)->required(),
      "Configuration file in JSON")(
      "logdir,l", boost::program_options::value<std::string>(&str_logdir),
      "Log directory (default is standard output)")(
      "nodename,n", boost::program_options::value<std::string>(&str_nodename),
      "Node name (default is the hostname)")(
      "timeout,t", boost::program_options::value<int>(&timeout),
      "Timeout in seconds (default is infinite)");

  try {
    boost::program_options::variables_map vm;
    boost::program_options::store(
        boost::program_options::command_line_parser(argc, argv).options(desc)
            .run(),
        vm);
    if (vm.count("help")) {
      std::cout << desc << std::endl;
      return EXIT_SUCCESS;
    }
    boost::program_options::notify(vm);
  } catch (const boost::program_options::error& e) {
    std::cerr << e.what() << std::endl << desc << std::endl;
    return EXIT_FAILURE;
  }

  if (timeout < 0) {
    LOG(ERROR) << "Wrong timeout: can't be negative!";
    return EXIT_FAILURE;
  }


  // Open configuration file

  std::ifstream f(str_conf);
  if (!f) {
    std::cerr << str_conf << ": No such file or directory\n";
    return EXIT_FAILURE;
  }
  Configuration configuration = read_configuration(f);

  // Check node name

  if (str_nodename.empty()) {
    str_nodename = boost::asio::ip::host_name();
  }

  // Configure log

  std::string logdir_postfix = "/" + str_nodename;
  str_logdir.append(logdir_postfix);

  std::ofstream log_stream(str_logdir);
  if (str_logdir == logdir_postfix) {
    Log::init("LSEB",
              Log::FromString(configuration.get<std::string>("LOG_LEVEL")));
  } else {
    Log::init("LSEB",
              Log::FromString(configuration.get<std::string>("LOG_LEVEL")),
              log_stream);
  }

  LOG(INFO) << configuration << std::endl;

  LOG(NOTICE) << "Node name: " << str_nodename;

  /************ Read configuration *****************/

  int id = -1;

  Configuration const& ep_child = configuration.get_child("ENDPOINTS");
  std::vector<Endpoint> endpoints;

  for (Configuration::const_iterator it = std::begin(ep_child), e =
    std::end(ep_child); it != e; ++it) {

    if (it->first == str_nodename) {
      id = std::distance(std::begin(ep_child), it);
    }

    endpoints.emplace_back(
      it->second.get < std::string > ("HOST"),
      it->second.get < std::string > ("PORT"));
  }

  if (id == -1) {
    LOG(ERROR) << "Wrong node name: can't find it in configuration file!";
    return EXIT_FAILURE;
  }

  int const max_fragment_size = configuration.get<int>(
      "GENERAL.MAX_FRAGMENT_SIZE");
  if (max_fragment_size <= 0 || max_fragment_size % sizeof(EventHeader)) {
    LOG(ERROR) << "Wrong MAX_FRAGMENT_SIZE: " << max_fragment_size;
    return EXIT_FAILURE;
  }

  int const bulk_size = configuration.get<int>("GENERAL.BULKED_EVENTS");
  if (bulk_size <= 0) {
    LOG(ERROR) << "Wrong BULKED_EVENTS: " << bulk_size;
    return EXIT_FAILURE;
  }

  int const credits = configuration.get<int>("GENERAL.CREDITS");
  if (credits < 1) {
    LOG(ERROR) << "Wrong CREDITS: " << credits;
    return EXIT_FAILURE;
  }

  /************** Memory allocation ******************/

  int const meta_size = sizeof(EventMetaData) * bulk_size * (credits * 2 + 1);
  int const data_size = max_fragment_size * bulk_size * (credits * 2 + 1);

  std::unique_ptr<unsigned char[]> const metadata_ptr(
      new unsigned char[meta_size]);
  std::unique_ptr<unsigned char[]> const data_ptr(new unsigned char[data_size]);

  MetaDataRange metadata_range(
      pointer_cast<EventMetaData>(metadata_ptr.get()),
      pointer_cast<EventMetaData>(metadata_ptr.get() + meta_size));
  DataRange data_range(data_ptr.get(), data_ptr.get() + data_size);

  /********* Generator, Controller and Accumulator **********/

  int const generator_frequency = configuration.get<int>("GENERATOR.FREQUENCY");
  assert(generator_frequency > 0);

  int const mean = configuration.get<int>("GENERATOR.MEAN");
  assert(mean > 0);

  int const stddev = configuration.get<int>("GENERATOR.STD_DEV");
  assert(stddev >= 0);

  LengthGenerator payload_size_generator(
      mean, stddev, max_fragment_size - sizeof(EventHeader));
  Generator generator(payload_size_generator, metadata_range, data_range, id);
  Controller controller(generator, metadata_range, generator_frequency);
  Accumulator accumulator(controller, metadata_range, data_range, bulk_size);

  /**************** Builder Unit and Readout Unit *****************/

  boost::lockfree::spsc_queue<iovec> free_local_data(credits);
  boost::lockfree::spsc_queue<iovec> ready_local_data(credits);

  BuilderUnit bu(free_local_data, ready_local_data, endpoints, bulk_size,
                 credits, max_fragment_size, id);

  ReadoutUnit ru(accumulator, free_local_data, ready_local_data, endpoints,
                 bulk_size, credits, id);

  std::thread bu_conn_th(&BuilderUnit::connect, &bu);
  std::thread ru_conn_th(&ReadoutUnit::connect, &ru);

  bu_conn_th.join();
  ru_conn_th.join();

  tcp_barrier(
    id,
    endpoints.size(),
    endpoints[0].hostname(),
    endpoints[0].port());

  std::thread bu_th(&BuilderUnit::run, &bu);
  std::thread ru_th(&ReadoutUnit::run, &ru);

  if (timeout) {
    std::this_thread::sleep_for(std::chrono::seconds(timeout));
  } else {
    bu_th.join();
    ru_th.join();
  }
  return EXIT_SUCCESS;
}
