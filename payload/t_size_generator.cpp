#include <iostream>

#include <boost/detail/lightweight_test.hpp>

#include "payload/size_generator.h"

using namespace lseb;

int main() {

  size_t const mean = 200;
  size_t const stddev = 50;
  size_t const min = 100;
  size_t const max = 400;

  SizeGenerator fixed_size(mean);

  // Check fixed size
  for(int i = 0; i < 1000; ++i){
    size_t payload = fixed_size.generate();
    BOOST_TEST_EQ(payload, mean);
  }

  SizeGenerator variable_size(mean, stddev);
  size_t const default_min = 0;
  size_t const default_max = mean + stddev * 5;

  // Check variable size
  for(int i = 0; i < 1000; ++i){
    size_t payload = variable_size.generate();
    BOOST_TEST(payload >= default_min);
    BOOST_TEST(payload <= default_max);
  }

  SizeGenerator variable_bounded_size(mean, stddev, max, min);

  // Check variable bounded size
  for(int i = 0; i < 1000; ++i){
    size_t payload = variable_bounded_size.generate();
    BOOST_TEST(payload >= min);
    BOOST_TEST(payload <= max);
  }

  boost::report_errors();
}

