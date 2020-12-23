#include "stringutil.hh"

namespace stringutil {

std::string toUpperCase(const std::string& str) {
  std::string result;
  std::transform(str.begin(), str.end(), std::back_inserter(result), (int (*)(int))std::toupper);
  return result;
}
std::string toLowerCase(const std::string& str) {
  std::string result;
  std::transform(str.begin(), str.end(), std::back_inserter(result), (int (*)(int))std::tolower);
  return result;
}

std::vector<std::string> split(const std::string& str, const std::string& separator) {
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

std::vector<std::string> splitIntoLines(const std::string& str) {
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

std::string replace(std::string str, const std::string& find_what, const std::string& replace_with) {
  std::string::size_type pos = 0;
  while ((pos = str.find(find_what, pos)) != std::string::npos) {
    str.erase(pos, find_what.length());
    str.insert(pos, replace_with);
    pos += replace_with.length();
  }
  return str;
}

std::string toString(double value, size_t precision) {
  std::stringstream ss;
  ss << std::setprecision(precision) << value;
  return ss.str();
}

std::string toHexString(uint32_t value, size_t width, const std::string& prefix) {
  std::stringstream ss;
  ss << prefix << std::setw(width) << std::setfill('0') << std::right << std::hex << value;
  return ss.str();
}

std::vector<int> toIntegerArray(const std::string& str) {
  std::vector<int> result{};
  for (const auto& entry : split(str, " ")) {
    result.push_back(std::stoi(entry));
  }
  return result;
}

std::vector<unsigned int> toUnsignedIntegerArray(const std::string& str) {
  std::vector<unsigned int> result;
  for (const auto& entry : split(str, " ")) {
    result.push_back(std::stoul(entry));
  }
  return result;
}

std::vector<unsigned char> toUnsignedCharArray(std::string str) {
  std::vector<unsigned char> result;
  str = stringutil::replace(str, "\n", "");
  str = stringutil::replace(str, "\r", "");
  str = stringutil::replace(str, "\t", " ");
  std::vector<std::string> stringArray = split(str, " ");
  for (size_t i = 0; i < stringArray.size(); i++) {
    if (stringArray[i].size() > 2 && stringArray[i][0] == '0' &&
        (stringArray[i][1] == 'x' || stringArray[i][1] == 'X')) {
      std::string element = stringutil::replace(stringutil::toLowerCase(stringArray[i]), "0x", "");
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
        ss >> std::hex >> avalue;
        result.push_back((uint8_t)avalue);
      }
    } else {
      result.push_back(std::stoi(stringArray[i]));
    }
  }
  return result;
}

std::vector<uint8_t> toUInt8Array(std::string str) {
  std::vector<uint8_t> result;

  str = stringutil::replace(str, "\n", "");
  str = stringutil::replace(str, "\r", "");
  str = stringutil::replace(str, "\t", " ");
  const std::vector<std::string> stringArray = split(str, " ");
  for (const auto& entry : stringArray) {
    if (entry.size() > 2 && entry[0] == '0' && (entry[1] == 'x' || entry[1] == 'X')) {
      std::string element = stringutil::replace(stringutil::toLowerCase(entry), "0x", "");
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
        ss >> std::hex >> avalue;
        result.push_back((uint8_t)avalue);
      }
    } else {
      result.push_back(static_cast<uint8_t>(std::stoi(entry)));
    }
  }
  return result;
}

std::string dumpAddress(void* address, size_t width) {
  std::stringstream ss;
  ss << "0x" << std::setw(width) << std::setfill('0') << std::hex << reinterpret_cast<uintptr_t>(address);
  return ss.str();
}

bool containsNumber(const std::string& str) {
  for (const auto ch : str) {
    if ('0' <= ch && ch <= '9') {
      return true;
    }
  }
  return false;
}

bool contain(const std::string& str, const std::string& searched_str) {
  return str.find(searched_str) != std::string::npos;
}

size_t indexOf(const std::string& str, const std::string& searched_str) { return str.find(searched_str); }

bool toBoolean(const std::string& str) {
  if (str == "true" || str == "TRUE" || str == "yes" || str == "YES") {
    return true;
  } else if (str == "false" || str == "FALSE" || str == "no" || str == "NO") {
    return false;
  } else {
    return false;
  }
}

}  // namespace stringutil
