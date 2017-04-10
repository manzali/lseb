#ifndef COMMON_EXCEPTION_H
#define COMMON_EXCEPTION_H

#include <string>
#include <stdexcept>

namespace lseb {

namespace exception {

namespace connector {

class generic_error : public std::runtime_error {
 public:
  explicit generic_error(std::string const& error)
      : std::runtime_error(error) {
  }
};

}

namespace acceptor {

class generic_error : public std::runtime_error {
 public:
  explicit generic_error(std::string const& error)
      : std::runtime_error(error) {
  }
};

}

namespace connection {

class generic_error : public std::runtime_error {
public:
  explicit generic_error(std::string const& error)
      : std::runtime_error(error) {
  }
};

}

namespace socket {

class generic_error : public std::runtime_error {
 public:
  explicit generic_error(std::string const& error)
      : std::runtime_error(error) {
  }
};

}

namespace configuration {

class generic_error : public std::runtime_error {
 public:
  explicit generic_error(std::string const& error)
      : std::runtime_error(error) {
  }
};

}

}

}

#endif
