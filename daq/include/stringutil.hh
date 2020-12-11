#ifndef GROWTHDAQ_STRINGUTIL_HH_
#define GROWTHDAQ_STRINGUTIL_HH_

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

namespace stringutil {

std::string toString(double value, size_t precision = 3) {
  std::stringstream ss;
  ss << std::setprecision(precision) << value;
  return ss.str();
}

std::string toHexString(uint32_t value, size_t width = 2, std::string_view prefix = "0x") {
  std::stringstream ss;
  ss << prefix << std::setw(width) << std::setfill('0') << std::right << std::hex << value;
  return ss.str();
}

std::vector<int> toIntegerArray(std::string_view str) {
  std::vector<int> result;
  std::vector<std::string> stringArray = stringutil::split(str, " ");
  for (unsigned int i = 0; i < stringArray.size(); i++) {
    result.push_back(stringutil::toInteger(stringArray[i]));
  }
  return result;
}

std::vector<unsigned int> toUnsignedIntegerArray(std::string str) {
  std::vector<unsigned int> result;
  std::vector<string> stringArray = stringutil::split(str, " ");
  for (unsigned int i = 0; i < stringArray.size(); i++) {
    result.push_back((unsigned int)stringutil::toInteger(stringArray[i]));
  }
  return result;
}

std::vector<unsigned char> toUnsignedCharArray(std::string str) {
  std::vector<unsigned char> result;
  str = stringutil::replace(str, "\n", "");
  str = stringutil::replace(str, "\r", "");
  str = stringutil::replace(str, "\t", " ");
  std::vector<string> stringArray = stringutil::split(str, " ");
  for (unsigned int i = 0; i < stringArray.size(); i++) {
    if (stringArray[i].size() > 2 && stringArray[i][0] == '0' &&
        (stringArray[i][1] == 'x' || stringArray[i][1] == 'X')) {
      string element = stringutil::replace(stringutil::toLowerCase(stringArray[i]), "0x", "");
      size_t elementLength = element.size();
      uint32_t avalue = 0;
      size_t o = 0;
      bool firstByte = true;

      while (o < element.size()) {
        std::stringstream ss;
        if (firstByte) {
          if (elementLength % 2 == 0) {  // even number
            ss << "0x" << element.substr(o, 2);
            o += 2;
          } else {  // odd number
            ss << "0x" << element.substr(o, 1);
            o++;
          }
          firstByte = false;
        } else {
          // read two characters
          ss << "0x" << element.substr(o, 2);
          o += 2;
        }
        ss >> hex >> avalue;
        result.push_back((uint8_t)avalue);
      }
    } else {
      unsigned char avalue = (unsigned char)stringutil::toUInt32(stringArray[i]);
      result.push_back(avalue);
    }
  }
  return result;
}

std::vector<uint8_t> toUInt8Array(std::string str) {
  std::vector<unsigned char> result;
  str = stringutil::replace(str, "\n", "");
  str = stringutil::replace(str, "\r", "");
  str = stringutil::replace(str, "\t", " ");
  std::vector<string> stringArray = stringutil::split(str, " ");
  for (unsigned int i = 0; i < stringArray.size(); i++) {
    if (stringArray[i].size() > 2 && stringArray[i][0] == '0' &&
        (stringArray[i][1] == 'x' || stringArray[i][1] == 'X')) {
      string element = stringutil::replace(stringutil::toLowerCase(stringArray[i]), "0x", "");
      size_t elementLength = element.size();
      uint32_t avalue = 0;
      size_t o = 0;
      bool firstByte = true;

      while (o < element.size()) {
        std::stringstream ss;
        if (firstByte) {
          if (elementLength % 2 == 0) {  // even number
            ss << "0x" << element.substr(o, 2);
            o += 2;
          } else {  // odd number
            ss << "0x" << element.substr(o, 1);
            o++;
          }
          firstByte = false;
        } else {
          // read two characters
          ss << "0x" << element.substr(o, 2);
          o += 2;
        }
        ss >> hex >> avalue;
        result.push_back((uint8_t)avalue);
      }
    } else {
      uint8_t uint8_value = (uint8_t)stringutil::toUInt32(stringArray[i]);
      result.push_back(uint8_value);
    }
  }
  return result;
}

double toDouble(char* str) { return toDouble(std::string(str)); }

double toDouble(const char* str) { return toDouble(std::string(str)); }

double toDouble(std::string str) {
  std::stringstream ss;
  ss << str;
  double avalue = 0;
  ss >> avalue;
  return avalue;
}

std::vector<std::string> split(std::string_view str, std::string_view separator) {
  std::vector<std::string> result;
  for (size_t i = 0, n = 0; i <= str.length(); i = n + 1) {
    n = str.find_first_of(separator, i);
    if (n == std::string::npos) {
      n = str.length();
    }
    if (i != n - 1) {
      result.push_back(str.substr(i, n - i));
    }
  }
  return result;
}

std::vector<std::string> splitIntoLines(std::string str) {
  std::stringstream ss;
  std::vector<std::string> result;
  ss << str;
  while (!ss.eof()) {
    std::string line;
    getline(ss, line);
    result.push_back(line);
  }
  return result;
}

std::string replace(std::string str, std::string find_what, std::string replace_with) {
  std::string::size_type pos = 0;
  while ((pos = str.find(find_what, pos)) != std::string::npos) {
    str.erase(pos, find_what.length());
    str.insert(pos, replace_with);
    pos += replace_with.length();
  }
  return str;
}

template <class T>
std::string join(const std::vector<T>& list, std::string_view separator) {
  std::stringstream ss;
  const size_t size = list.size();
  for (size_t i = 0; i < size; i++) {
    if constexpr (std::is_same<T, uint8_t>) {
      ss << static_cast<uint32_t>(list.at(i));
    } else if constexpr (std::is_same<T, int8_t>) {
      ss << static_cast<int32_t>(list.at(i));
    } else if constexpr (std::is_same<T, bool>) {
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

std::string dumpAddress(void* address, unsigned int width = 4) {
  std::stringstream ss;
  ss << "0x" << std::setw(width) << std::setfill('0') << std::hex << (long long)address;
  return ss.str();
}

bool containsNumber(std::string_view str) {
  for (const auto ch : str) {
    if ('0' <= ch && ch <= '9') {
      return true;
    }
  }
  return false;
}

bool contain(std::string_view str, std::string_view searched_str) {
  return str.find(searched_str) != std::string::npos;
}

size_t indexOf(std::string_view str, std::string_view searched_str) { return str.find(searched_str); }

std::string toUpperCase(std::string str) {
  std::string result;
  std::transform(str.begin(), str.end(), std::back_inserter(result), (int (*)(int))std::toupper);
  return result;
}
std::string toLowerCase(std::string str) {
  std::string result;
  std::transform(str.begin(), str.end(), std::back_inserter(result), (int (*)(int))std::tolower);
  return result;
}

/** Converts string to boolean.
 * true or yes => true.
 * false or no => false.
 * otherwise => false.
 */
bool toBoolean(std::string str) {
  if (str == "true" || str == "TRUE" || str == "yes" || str == "YES") {
    return true;
  } else if (str == "false" || str == "FALSE" || str == "no" || str == "NO") {
    return false;
  } else {
    return false;
  }
}
CxxUtilities::Date parseYYYYMMDD_HHMMSS(std::string yyyymmdd_hhmmss) {
  return CxxUtilities::Date::parseYYYYMMDD_HHMMSS(yyyymmdd_hhmmss);
}

}  // namespace stringutil

#endif
