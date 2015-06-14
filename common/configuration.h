#ifndef COMMON_CONFIGURATION_H
#define COMMON_CONFIGURATION_H

#include <iosfwd>
#include <string>
#include <iostream>
#include <fstream>
#include <stdexcept>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/exceptions.hpp>
#include <boost/property_tree/json_parser.hpp>

typedef boost::property_tree::ptree Configuration;
typedef boost::property_tree::ptree_error Configuration_error;

inline Configuration read_configuration(std::istream& is) {
  Configuration configuration;
  read_json(is, configuration);
  return configuration;
}

inline Configuration read_configuration(std::string const& file) {
  std::ifstream file_is(file.c_str());
  if (!file_is) {
    throw std::runtime_error("Cannot open configuration file \"" + file + '"');
  }
  return read_configuration(file_is);
}

inline std::ostream& operator<<(
  std::ostream& os,
  Configuration const& configuration) {
  write_json(os, configuration);
  return os;
}

#endif
