#include <thread>

#include <boost/lockfree/spsc_queue.hpp>
#include <boost/program_options.hpp>

#include <cassert>

#include "bu/builder_unit.h"
#include "ru/readout_unit.h"

#include "common/log.hpp"
#include "common/configuration.h"
#include "common/dataformat.h"

#include "transport/endpoints.h"

using namespace lseb;

int main(int argc, char* argv[]) {

  int id;
  std::string str_conf;

  boost::program_options::options_description desc("Options");

  desc.add_options()("help,h", "Print help messages.")(
    "id,i",
    boost::program_options::value<int>(&id)->required(),
    "Process ID.")(
    "configuration,c",
    boost::program_options::value<std::string>(&str_conf)->required(),
    "Configuration JSON file.");

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

  std::ifstream f(str_conf);
  if (!f) {
    std::cerr << str_conf << ": No such file or directory\n";
    return EXIT_FAILURE;
  }

  Configuration configuration = read_configuration(f);

  Log::init(
    "LSEB",
    Log::FromString(configuration.get<std::string>("LOG_LEVEL")));

  LOG(INFO) << configuration << std::endl;

  /************ Read configuration *****************/

  std::vector<Endpoint> const endpoints = get_endpoints(
    configuration.get_child("ENDPOINTS"));
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
  if (credits <= 0) {
    LOG(ERROR) << "Wrong CREDITS: " << credits;
    return EXIT_FAILURE;
  }

  /************** Memory allocation ******************/

  int const meta_size = sizeof(EventMetaData) * bulk_size * credits;
  int const data_size = max_fragment_size * bulk_size * credits;

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
    bulk_size,
    endpoints.size());

  /**************** Builder Unit and Readout Unit *****************/

  boost::lockfree::spsc_queue<iovec> free_local_data(credits);
  boost::lockfree::spsc_queue<iovec> ready_local_data(credits);

  BuilderUnit bu(
    free_local_data,
    ready_local_data,
    endpoints,
    bulk_size,
    credits,
    max_fragment_size,
    id);
  std::thread bu_th(&BuilderUnit::operator(), &bu);

  ReadoutUnit ru(
    accumulator,
    free_local_data,
    ready_local_data,
    endpoints,
    bulk_size,
    credits,
    id);
  std::thread ru_th(&ReadoutUnit::operator(), &ru);

  bu_th.join();
  ru_th.join();

  return EXIT_SUCCESS;
}
