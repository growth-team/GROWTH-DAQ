#include "timeutil.hh"

namespace timeutil {

std::string getCurrentTimeAsString(const std::string& format) {
  const auto time = std::chrono::system_clock::now();
  const auto t = std::chrono::system_clock::to_time_t(time);
  const auto datetime = *localtime(&t);
  std::ostringstream oss;
  oss << std::put_time(&datetime, format.c_str());
  return oss.str();
}

std::string getCurrentTimeYYYYMMDD_HHMM() { return getCurrentTimeAsString("%Y%m%d_%H%M"); }

std::string getCurrentTimeYYYYMMDD_HHMMSS() { return getCurrentTimeAsString("%Y%m%d_%H%M%S"); }

u64 getUNIXTimeAsUInt64() {
  return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}
}  // namespace timeutil
