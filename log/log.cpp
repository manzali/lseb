#include "log.hpp"

#include <iostream>
#include <fstream>

#include <boost/thread.hpp> // shared_mutex

#include <boost/core/null_deleter.hpp>

#include <boost/log/sinks.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/support/date_time.hpp> // TimeStamp
#include <boost/log/attributes/clock.hpp>

namespace async_log {

namespace {

boost::shared_mutex init_mutex;
bool init_flag = false;  // protected by the shared_mutex

namespace sinks = boost::log::sinks;

typedef sinks::asynchronous_sink<sinks::text_ostream_backend> console_sink_t;
typedef sinks::asynchronous_sink<sinks::text_file_backend> file_sink_t;

boost::shared_ptr<console_sink_t> console_sink;
boost::shared_ptr<file_sink_t> file_sink;

// function protected by the anonymous namespace
void _add_console() {
  if (!init_flag) {
    throw std::runtime_error("log uninitialized\n");
  } else if (console_sink) {
    throw std::runtime_error("log console already added\n");
  } else {
    // create new console sink
    console_sink.reset(
        new console_sink_t(),
        [](console_sink_t* s) {s->flush();});
    // format it
    console_sink->set_formatter(
        boost::log::expressions::stream << "["
            << boost::log::expressions::format_date_time<
                boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S")
            << "] ["
            << boost::log::expressions::attr<severity_level>("Severity") << "] "
            << boost::log::expressions::smessage);
    // add output stream to the sink
    console_sink->locked_backend()->add_stream(
        boost::shared_ptr<std::ostream>(&std::clog, boost::null_deleter()));
    // add the console sink to the core
    boost::log::core::get()->add_sink(console_sink);
  }
}

void _add_file(std::string const& filename, size_t bytes) {
  if (!init_flag) {
    throw std::runtime_error("log uninitialized\n");
  } else if (file_sink) {
    throw std::runtime_error("log file already added\n");
  } else {
    // create file backend with rotation and date inside name
    boost::shared_ptr<boost::log::sinks::text_file_backend> backend =
        boost::make_shared<boost::log::sinks::text_file_backend>(
            boost::log::keywords::file_name = filename
                + std::string("_%Y-%m-%d_%H-%M-%S.%5N.log"),
            boost::log::keywords::rotation_size = bytes);
    // create new file sink
    file_sink.reset(new file_sink_t(backend), [](file_sink_t* s) {s->flush();});
    // format it
    file_sink->set_formatter(
        boost::log::expressions::stream << "["
            << boost::log::expressions::format_date_time<
                boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S")
            << "] ["
            << boost::log::expressions::attr<severity_level>("Severity") << "] "
            << boost::log::expressions::smessage);
    // add the file sink to the to the core
    boost::log::core::get()->add_sink(file_sink);
  }
}

}

void init() {
  boost::lock_guard<boost::shared_mutex> lock(init_mutex);  // one write, no reads
  if (!init_flag) {
    // add timestamp to the log
    boost::log::core::get()->add_global_attribute(
        boost::log::aux::default_attribute_names::timestamp(),
        boost::log::attributes::local_clock());
    init_flag = true;
  } else {
    throw std::runtime_error("log already initialized\n");
  }
}

void add_console() {
  boost::lock_guard<boost::shared_mutex> lock(init_mutex);  // one write, no reads
  _add_console();
}

void add_console(severity_level const& level) {
  boost::lock_guard<boost::shared_mutex> lock(init_mutex);  // one write, no reads
  _add_console();
  console_sink->set_filter(boost::log::trivial::severity >= level);
}

void remove_console() {
  boost::lock_guard<boost::shared_mutex> lock(init_mutex);  // one write, no reads
  if (!init_flag) {
    throw std::runtime_error("log uninitialized\n");
  } else if (!console_sink) {
    throw std::runtime_error("missing log console\n");
  } else {
    boost::log::core::get()->remove_sink(console_sink);
    console_sink.reset();
  }
}

void add_file(std::string const& filename, size_t bytes) {
  boost::lock_guard<boost::shared_mutex> lock(init_mutex);  // one write, no reads
  _add_file(filename, bytes);
}

void add_file(
    std::string const& filename,
    size_t bytes,
    severity_level const& level) {
  boost::lock_guard<boost::shared_mutex> lock(init_mutex);  // one write, no reads
  _add_file(filename, bytes);
  file_sink->set_filter(boost::log::trivial::severity >= level);
}

void remove_file() {
  boost::lock_guard<boost::shared_mutex> lock(init_mutex);  // one write, no reads
  if (!init_flag) {
    throw std::runtime_error("log uninitialized\n");
  } else if (!file_sink) {
    throw std::runtime_error("missing log file\n");
  } else {
    boost::log::core::get()->remove_sink(file_sink);
    file_sink.reset();
  }
}

log::log(severity_level level)
    : m_level(level) {

}

log::~log() {
  boost::shared_lock<boost::shared_mutex> lock(init_mutex);  // multiple reads, no write
  if (init_flag) {
    boost::log::sources::severity_logger<severity_level> lg;
    BOOST_LOG_SEV(lg, m_level)<<m_s.str();
  }
  else {
    throw std::runtime_error("log uninitialized\n");
  }

}

}
