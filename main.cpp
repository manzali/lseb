#include <thread>
#include <iostream>
#include <fstream>

#include <boost/program_options.hpp>

#include <cassert>
#include <signal.h>

#include "bu/builder_unit.h"
#include "ru/readout_unit.h"

#include "common/log.hpp"
#include "common/configuration.h"
#include "common/dataformat.h"
#include "common/local_ip.h"

#ifdef HAVE_HYDRA
	#include "launcher/hydra_launcher.hpp"
#endif //HAVE_HYDRA

#include "transport/endpoints.h"

using namespace lseb;

int main(int argc, char* argv[]) {

  int id;
  std::string str_conf;
  int timeout = 0;

  boost::program_options::options_description desc("Options");

  desc.add_options()("help,h", "Print help messages.")(
    "id,i",
    boost::program_options::value<int>(&id)->required(),
    "Process ID.")(
    "configuration,c",
    boost::program_options::value<std::string>(&str_conf)->required(),
    "Configuration JSON file.")(
    "timeout,t",
    boost::program_options::value<int>(&timeout),
    "Timeout in seconds (default is infinite)");

  try {
    boost::program_options::variables_map vm;
    boost::program_options::store(
      boost::program_options::command_line_parser(argc, argv).options(desc).run(),
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

  std::ifstream f(str_conf);
  if (!f) {
    std::cerr << str_conf << ": No such file or directory\n";
    return EXIT_FAILURE;
  }

  Configuration configuration = read_configuration(f);

  /*
  std::ofstream log_file("/tmp/tridas_log.txt");
  Log::init(
    "LSEB",
    Log::FromString(configuration.get<std::string>("LOG_LEVEL")),
    log_file);
*/

  Log::init(
    "LSEB",
    Log::FromString(configuration.get<std::string>("LOG_LEVEL")));

  LOG(INFO) << configuration << std::endl;
  
  /****** Setup Launcher / exchange addresses ******/
  #ifdef HAVE_HYDRA
    //extract from config
    std::string iface = configuration.get_child("NETWORK").get<std::string>("IFACE");
    int port = configuration.get_child("NETWORK").get<int>("PORT");
    int range = configuration.get_child("NETWORK").get<int>("RANGE");
    if (iface.empty())
      iface = "ib0";
    LOG(DEBUG) << "Using iface = " << iface << ", port = " << port << ", range = " << range;
  
    //get ip
    std::string ip = get_local_ip(iface);
  
    //exchange
    HydraLauncher launcher;
    launcher.initialize(argc,argv);
    char portStr[8];
    sprintf(portStr,"%d",port+launcher.getRank()%range);
    launcher.set("ip",ip);
    launcher.set("port",portStr);
    launcher.commit();
    launcher.barrier();
  
    //extract id
    id = launcher.getRank();
  #endif //HAVE_HYDRA

  /************ Read configuration *****************/

  #ifdef HAVE_HYDRA
    std::vector<Endpoint> const endpoints = get_endpoints(launcher);
  #else //HAVE_HYDRA
    std::vector<Endpoint> const endpoints = get_endpoints(
      configuration.get_child("ENDPOINTS"));
  #endif //HAVE_HYDRA
  if (id < 0 || id >= endpoints.size()) {
    LOG(ERROR) << "Wrong ID: " << id;
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
    mean,
    stddev,
    max_fragment_size - sizeof(EventHeader));
  Generator generator(payload_size_generator, metadata_range, data_range, id);
  Controller controller(generator, metadata_range, generator_frequency);
  Accumulator accumulator(
    controller,
    metadata_range,
    data_range,
    bulk_size);

  /**************** Builder Unit and Readout Unit *****************/
  BuilderUnit bu(
    endpoints.size(),
    bulk_size,
    credits,
    max_fragment_size,
    id);

  ReadoutUnit ru(
    accumulator,
    bulk_size,
    credits,
    id);

  std::thread bu_conn_th(&BuilderUnit::connect, &bu, endpoints);
  std::thread ru_conn_th(&ReadoutUnit::connect, &ru, endpoints);

  bu_conn_th.join();
  ru_conn_th.join();

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
