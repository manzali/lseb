#ifndef COMMON_LOG_HPP
#define COMMON_LOG_HPP

#include <iostream>
#include <sstream>
#include <string>
#include <mutex>

#include <ctime>
#include <cassert>

#include <syslog.h>

#define LOG(level) \
  if (lseb::Log::level > lseb::Log::BaseLevel()) ; \
  else lseb::Log(lseb::Log::level).tmp_stream()

namespace lseb {

namespace detail {

int const BUFSIZE = 200;

inline std::string Now() {
  auto now = std::chrono::system_clock::now();
  auto in_time_t = std::chrono::system_clock::to_time_t(now);
  char tmdescr[BUFSIZE] = { 0 };
  const char fmt[] = "%F - %T";
  std::strftime(tmdescr, sizeof(tmdescr) - 1, fmt, std::localtime(&in_time_t));
  return std::string(tmdescr);
}

inline void log_to_syslog(
  std::istream& input_stream,
  std::mutex& mx,
  int level,
  std::string const& level_str) {
  std::unique_lock<std::mutex> l(mx);
  bool continuation = false;
  for (std::string s; getline(input_stream, s);) {
    std::ostringstream os;
    os << level_str << (continuation ? "+ " : ": ") << s;
    syslog(level, "%s", os.str().c_str());
    continuation = true;
  }
}

inline void log_to_stream(
  std::ostream& output_stream,
  std::istream& input_stream,
  std::mutex& mx,
  std::string const& ident,
  std::string const& level) {
  std::ostringstream os;
  std::string const now = Now();
  bool continuation = false;
  for (std::string s; getline(input_stream, s);) {
    if (continuation)
      os << '\n';
    os << now << ' ' << ident << ": ";
    os << level << (continuation ? "+ " : ": ") << s;
    continuation = true;
  }
  std::unique_lock<std::mutex> l(mx);
  output_stream << os.str().c_str() << std::endl;
}

}

class Log {
 public:
  enum Level {
    ERROR, WARNING, NOTICE, INFO, DEBUG
  };

 public:
  Log(Level level = NOTICE)
      :
        m_level(level) {
  }
  ~Log() {
    StaticMembers& sm(static_members());
    assert(!sm.ident.empty() && "Did you forget to call Log::Init()?");
    if (sm.use_syslog) {
      detail::log_to_syslog(
        m_tmp_stream,
        sm.mx,
        ToSyslog(m_level),
        ToString(m_level));
    } else {
      detail::log_to_stream(
        *sm.log_stream,
        m_tmp_stream,
        sm.mx,
        sm.ident,
        ToString(m_level));
    }
  }
  Log(const Log&) = delete;
  void operator=(const Log&) = delete;
  std::ostream& tmp_stream() {
    return m_tmp_stream;
  }
  ;

 private:
  Level m_level;
  std::stringstream m_tmp_stream;

  struct StaticMembers {
    std::string ident;
    bool use_syslog;
    std::mutex mx;
    std::ostream* log_stream;
    Level base_level;
    StaticMembers()
        :
          use_syslog(false),
          log_stream(&std::clog),
          base_level(INFO) {
    }
    StaticMembers(const StaticMembers&) = delete;
    void operator=(const StaticMembers&) = delete;
  };

  static StaticMembers& static_members() {
    static StaticMembers sm;
    return sm;
  }

 public:

  static void init(
    std::string const& ident,
    Level base_level = INFO,
    std::ostream& log_stream = std::clog) {
    assert(!ident.empty() && "The ident string must be non empty");
    assert(
      base_level >= ERROR && base_level <= DEBUG && "Invalid logging level");
    assert(log_stream.good() && "The log stream is not in a good state");
    StaticMembers& sm(static_members());
    sm.ident = ident;
    sm.base_level = base_level;
    sm.log_stream = &log_stream;
  }
  static Level BaseLevel() {
    return static_members().base_level;
  }
  static void UseSyslog(int option = 0, int facility = LOG_USER) {
    StaticMembers& sm(static_members());
    sm.use_syslog = true;
    openlog(sm.ident.c_str(), option, facility);
  }
  static int ToSyslog(Level level) {
    static int const syslog_level[] = {
    LOG_ERR, LOG_WARNING, LOG_NOTICE, LOG_INFO, LOG_DEBUG };
    assert(level >= ERROR && level <= DEBUG);
    return syslog_level[level];
  }
  static std::string ToString(Level level) {
    static const char* const buffer[] = { "ERROR", "WARNING", "NOTICE", "INFO", "DEBUG" };
    assert(level >= ERROR && level <= DEBUG);
    return buffer[level];
  }
  static Level FromString(const std::string& level) {
    if (level == "DEBUG")
      return DEBUG;
    if (level == "INFO")
      return INFO;
    if (level == "NOTICE")
      return NOTICE;
    if (level == "WARNING")
      return WARNING;
    if (level == "ERROR")
      return ERROR;

    assert(!"Unknown logging level");
  }
};

}

#endif
