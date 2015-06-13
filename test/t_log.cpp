#include <fstream>

#include "common/log.hpp"

int main() {

  //lseb::Log::UseSyslog();

  lseb::Log::init("t_log", lseb::Log::DEBUG);

  LOG(DEBUG) << "This is a DEBUG message";
  LOG(INFO) << "This is an INFO message";
  LOG(WARNING) << "This is a WARNING message\non two lines";
  LOG(ERROR) << "This is an ERROR message";

  std::ofstream f("/tmp/lseb_log.txt");
  lseb::Log::init("t_log", lseb::Log::DEBUG, f);

  LOG(DEBUG) << "This is a DEBUG message in a log file";

}
