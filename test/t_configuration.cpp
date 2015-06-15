#include <cassert>
#include <fstream>
#include <utility>
#include <cstdlib>

#include <boost/detail/lightweight_test.hpp>

#include "common/configuration.h"

template<typename T, typename U>
std::ostream& operator<<(std::ostream& os, std::pair<T, U> const& entry) {
  return std::cout << entry.first << " = " << entry.second;
}

int main(int argc, char* argv[]) {
  assert(argc == 2 && "usage: t_Configuration <json-file>");
  std::ifstream f(argv[1]);
  if (!f) {
    std::cerr << argv[1] << ": No such file or directory\n";
    return EXIT_FAILURE;
  }

  Configuration configuration = read_configuration(f);

  std::cout << "*** Configuration dump ***\n\n" << configuration << '\n';

  // key1
  BOOST_TEST_EQ(configuration.get<int>("key1.integer"), 123);
  BOOST_TEST_EQ(configuration.get<double>("key1.double"), 4.56);
  BOOST_TEST_EQ(configuration.get<bool>("key1.bool"), true);

  // key2
  Configuration const& child = configuration.get_child("key2");
  for(auto& item : child){
    BOOST_TEST_EQ(item.second.get<std::string>("array"), "value");
  }

  // key3
  BOOST_TEST_EQ(configuration.get<std::string>("key3"), "abc");

  // missing key
  BOOST_TEST_THROWS(configuration.get<std::string>("key4"), Configuration_error);

  boost::report_errors();
}
