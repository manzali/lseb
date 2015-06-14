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

  {
    std::string const key("key1");
    int const expected = 123;
    BOOST_TEST_EQ(configuration.get<int>(key), expected);
  }

/*
  {  // missing section
    std::string const key("NPMTFLOOR");
    BOOST_TEST_THROWS(configuration.get<int>(key), Configuration_error);
  }
  */

boost::report_errors();
}
