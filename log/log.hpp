#ifndef LOG_LOG_HPP
#define LOG_LOG_HPP

#include <sstream>

#include <boost/log/trivial.hpp>
#include <boost/log/sources/global_logger_storage.hpp>

// Define log macros
#define LOG_TRACE async_log::log(async_log::severity_level::trace)
#define LOG_DEBUG async_log::log(async_log::severity_level::debug)
#define LOG_INFO async_log::log(async_log::severity_level::info)
#define LOG_WARNING async_log::log(async_log::severity_level::warning)
#define LOG_ERROR async_log::log(async_log::severity_level::error)
#define LOG_FATAL async_log::log(async_log::severity_level::fatal)

namespace async_log {

typedef boost::log::trivial::severity_level severity_level;

void init();

void add_console();
void add_console(severity_level const& level);
void remove_console();

void add_file(std::string const& file, size_t bytes);
void add_file(
    std::string const& file,
    size_t bytes,
    severity_level const& level);
void remove_file();

class log {

 private:

  std::stringstream m_s;
  severity_level m_level;

 public:

  log(severity_level level);
  ~log();

  log(log const&) = delete;
  void operator=(log const&) = delete;

  template<typename T>
  log& operator <<(T const& t) {
    m_s << t;
    return *this;
  }

  log& operator<<(std::ostream& (*f)(std::ostream& o)) {
    m_s << f;
    return *this;
  }

  log& operator<<(std::ostream& (*f)(std::ios& o)) {
    m_s << f;
    return *this;
  }

  log& operator<<(std::ostream& (*f)(std::ios_base& o)) {
    m_s << f;
    return *this;
  }

};

}

#endif
