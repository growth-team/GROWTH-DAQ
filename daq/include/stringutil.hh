#ifndef GROWTHDAQ_STRINGUTIL_HH_
#define GROWTHDAQ_STRINGUTIL_HH_

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>
#include <algorithm>

namespace stringutil {

std::string toUpperCase(const std::string &str);
std::string toLowerCase(const std::string &str);
std::vector<std::string> split(const std::string &str,
		const std::string &separator);
std::vector<std::string> splitIntoLines(const std::string &str);
std::string replace(std::string str, const std::string &find_what,
		const std::string &replace_with);
template<class T>
std::string join(const std::vector<T> &list, const std::string &separator) {
	std::stringstream ss;
	const size_t size = list.size();
	for (size_t i = 0; i < size; i++) {
		if constexpr (std::is_same<T, uint8_t>::value) {
			ss << static_cast<uint32_t>(list.at(i));
		} else if constexpr (std::is_same<T, int8_t>::value) {
			ss << static_cast<int32_t>(list.at(i));
		} else if constexpr (std::is_same<T, bool>::value) {
			ss << (list.at(i) ? "true" : "false");
		} else {
			ss << list.at(i);
		}
		if (i != size - 1) {
			ss << separator;
		}
	}
	return ss.str();
}

std::string toString(double value, size_t precision = 3);
std::string toHexString(uint32_t value, size_t width = 2,
		const std::string &prefix = "0x");
std::vector<int> toIntegerArray(const std::string &str);
std::vector<unsigned int> toUnsignedIntegerArray(const std::string &str);
std::vector<unsigned char> toUnsignedCharArray(std::string str);
std::vector<uint8_t> toUInt8Array(std::string str);
std::string dumpAddress(void *address, size_t width = 4);
bool containsNumber(const std::string &str);
bool contain(const std::string &str, const std::string &searched_str);
size_t indexOf(const std::string &str, const std::string &searched_str);

/** Converts string to boolean.
 * true or yes => true.
 * false or no => false.
 * otherwise => false.
 */
bool toBoolean(const std::string &str);
}  // namespace stringutil

#endif
