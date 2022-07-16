#ifndef GROWTHDAQ_TIMEUTIL_HH_
#define GROWTHDAQ_TIMEUTIL_HH_

#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>
#include "types.h"
namespace timeutil {

std::string getCurrentTimeAsString(const std::string &format =
		"%Y-%m-%d %H:%M:%S");
std::string getCurrentTimeYYYYMMDD_HHMM();
std::string getCurrentTimeYYYYMMDD_HHMMSS();
u64 getUNIXTimeAsUInt64();
}  // namespace timeutil

#endif
