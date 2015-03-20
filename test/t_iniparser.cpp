#include <iostream>
#include <sstream>

#include <cassert>

#include "common/boost_lightweight_test.hpp"
#include "common/iniparser.hpp"

int main(int argc, char* argv[]) {

  assert(argc == 2 && "The filename is required as parameter!");

  lseb::Parser p(argv[1]);

  BOOST_TEST_EQ(p.top()["a0"], "qqq");
  BOOST_TEST_EQ(p.top()("s1")["a1"], "123");
  BOOST_TEST_EQ(p.top()("s1")("s2")["a2"], "456");
  BOOST_TEST_EQ(p.top()("s3")["a3"], "true");

  std::stringstream out;
  p.dump(out);

  std::cout << out.str() << std::endl;

  boost::report_errors();

  return 0;
}
