#include "spdlog/spdlog.h"
#include "spdlog/fmt/fmt.h"
#include "types.h"
#include "stringutil.hh"

#include <picojson.h>

// ZeroMQ
#include <unistd.h>
#include <string>
#include <zmq.hpp>

#include <cxxopts.hpp>

#include <iostream>
#include <sstream>
#include <string>
#include <optional>
#include <tuple>

class MessageClient {
 public:
  MessageClient(const std::string hostname) : context_(1), socket_(context_, zmq::socket_type::req) {
    spdlog::info("Connecting to grwoth_daq's MessageServer (TCP port {})", TCPPortNumber);
    std::stringstream ss{};
    ss << "tcp://" << hostname << ":" << TCPPortNumber;
    socket_.setsockopt(ZMQ_RCVTIMEO, &TimeOutInMilisecond, sizeof(TimeOutInMilisecond));
    socket_.setsockopt(ZMQ_SNDTIMEO, &TimeOutInMilisecond, sizeof(TimeOutInMilisecond));
    int on = 1;
    socket_.setsockopt(ZMQ_IMMEDIATE, &on, sizeof(on));

    try {
      socket_.connect(ss.str().c_str());
    } catch (...) {
      spdlog::error("Failed to connect to {}", ss.str());
      exit(1);
    }
  }

  bool executeCommandPing(const bool dumpRawJsonReply) {
    picojson::object object;
    object["command"] = picojson::value("ping");
    try {
      sendCommand(object);
    } catch (...) {
      spdlog::error("Failed to send command");
      return false;
    }

    const auto maybeReplyJsonObject = receiveReply();
    if (maybeReplyJsonObject) {
      const auto replyJsonObject = maybeReplyJsonObject.value();
      if (dumpRawJsonReply) {
        std::cout << picojson::value(replyJsonObject).serialize() << std::endl;
      } else {
        // Dump status only
        std::cout << replyJsonObject.at("status").to_str() << std::endl;
      }
    } else {
      spdlog::error("Replied message was not a valid JSON object");
    }
    return true;
  }

  bool executeCommandRead(const std::string& command, const u32 address, const bool dumpRawJsonReply) {
    picojson::object object;
    if (command == "read16") {
      object["command"] = picojson::value("registerRead16");
    } else if (command == "read32") {
      object["command"] = picojson::value("registerRead32");
    } else {
      throw std::runtime_error(fmt::format("Invalid command {}", command));
    }
    object["address"] = picojson::value(static_cast<f64>(address));
    spdlog::info("Reading address 0x{:08x}", address);
    sendCommand(object);

    const auto maybeReplyJsonObject = receiveReply();
    if (maybeReplyJsonObject) {
      const auto replyJsonObject = maybeReplyJsonObject.value();
      if (dumpRawJsonReply) {
        std::cout << picojson::value(replyJsonObject).serialize() << std::endl;
      } else {
        // Dump read value only
        const auto value = static_cast<u32>(replyJsonObject.at("registerValue").get<f64>());
        if (command == "read16") {
          std::cout << fmt::format("0x{:04x}", value) << '\n';
        } else if (command == "read32") {
          std::cout << fmt::format("0x{:08x}", value) << '\n';
        } else {
          throw std::runtime_error(fmt::format("Invalid command {}", command));
        }
      }
    } else {
      spdlog::error("Replied message was not a valid JSON object");
    }
    return true;
  }

  bool executeCommandWrite(const std::string& command, const u32 address, const u32 value, const bool dumpRawJsonReply) {
    picojson::object object;
    if (command == "write16") {
      object["command"] = picojson::value("registerWrite16");
    } else if (command == "write32") {
      object["command"] = picojson::value("registerWrite32");
    } else {
      throw std::runtime_error(fmt::format("Invalid command {}", command));
    }
    object["address"] = picojson::value(static_cast<f64>(address));
    object["value"] = picojson::value(static_cast<f64>(value));
    if (command == "write16") {
      spdlog::info("Writing value 0x{:04x} to address 0x{:08x}", value, address);
    } else if (command == "write32") {
      spdlog::info("Writing value 0x{:08x} to address 0x{:08x}", value, address);
    } else {
      throw std::runtime_error(fmt::format("Invalid command {}", command));
    }
    sendCommand(object);

    const auto maybeReplyJsonObject = receiveReply();
    if (maybeReplyJsonObject) {
      const auto replyJsonObject = maybeReplyJsonObject.value();
      if (dumpRawJsonReply) {
        std::cout << picojson::value(replyJsonObject).serialize() << std::endl;
      } else {
        // Dump read value only
        std::cout << replyJsonObject.at("status").to_str() << std::endl;
      }
    } else {
      spdlog::error("Replied message was not a valid JSON object");
    }
    return true;
  }

  bool executeCommandStartNewFile(const std::string& command, const bool dumpRawJsonReply) {
    picojson::object object;
    object["command"] = picojson::value("startNewOutputFile");
    sendCommand(object);

    const auto maybeReplyJsonObject = receiveReply();
    if (maybeReplyJsonObject) {
      const auto replyJsonObject = maybeReplyJsonObject.value();
      if (dumpRawJsonReply) {
        std::cout << picojson::value(replyJsonObject).serialize() << std::endl;
      } else {
        // Dump read value only
        std::cout << replyJsonObject.at("status").to_str() << std::endl;
      }
    } else {
      spdlog::error("Replied message was not a valid JSON object");
    }
    return true;
  }

  bool executeCommandStop(const std::string& command, const bool dumpRawJsonReply) {
    picojson::object object;
    object["command"] = picojson::value("stop");
    sendCommand(object);

    const auto maybeReplyJsonObject = receiveReply();
    if (maybeReplyJsonObject) {
      const auto replyJsonObject = maybeReplyJsonObject.value();
      if (dumpRawJsonReply) {
        std::cout << picojson::value(replyJsonObject).serialize() << std::endl;
      } else {
        // Dump read value only
        std::cout << replyJsonObject.at("status").to_str() << std::endl;
      }
    } else {
      spdlog::error("Replied message was not a valid JSON object");
    }
    return true;
  }

  bool executeCommandPause(const std::string& command, const bool dumpRawJsonReply) {
    picojson::object object;
    object["command"] = picojson::value("pause");
    sendCommand(object);

    const auto maybeReplyJsonObject = receiveReply();
    if (maybeReplyJsonObject) {
      const auto replyJsonObject = maybeReplyJsonObject.value();
      if (dumpRawJsonReply) {
        std::cout << picojson::value(replyJsonObject).serialize() << std::endl;
      } else {
        // Dump read value only
        std::cout << replyJsonObject.at("status").to_str() << std::endl;
      }
    } else {
      spdlog::error("Replied message was not a valid JSON object");
    }
    return true;
  }

  bool executeCommandResume(const std::string& command, const bool dumpRawJsonReply) {
    picojson::object object;
    object["command"] = picojson::value("resume");
    sendCommand(object);

    const auto maybeReplyJsonObject = receiveReply();
    if (maybeReplyJsonObject) {
      const auto replyJsonObject = maybeReplyJsonObject.value();
      if (dumpRawJsonReply) {
        std::cout << picojson::value(replyJsonObject).serialize() << std::endl;
      } else {
        // Dump read value only
        std::cout << replyJsonObject.at("status").to_str() << std::endl;
      }
    } else {
      spdlog::error("Replied message was not a valid JSON object");
    }
    return true;
  }
    
  bool parseAndExecute(const std::vector<std::string>& arguments, bool dumpRawJsonReply) {
    auto itr = arguments.begin();
    while (itr < arguments.end()) {
      const auto& command = *itr;
      const bool commandResult = [&]() {
        if (command == "ping") {
          return executeCommandPing(dumpRawJsonReply);
        } else if (command == "start_new_file") {
          return executeCommandStartNewFile(command, dumpRawJsonReply);
        } else if (command == "stop") {
          return executeCommandStop(command, dumpRawJsonReply);
        } else if (command == "pause") {
          return executeCommandPause(command, dumpRawJsonReply);
        } else if (command == "resume") {
          return executeCommandResume(command, dumpRawJsonReply);
        } else if (command == "read16" || command == "read32") {
          itr++;
          if (itr == arguments.end()) {
            throw std::runtime_error("missing address argument after read command");
          }
          const u32 address = stringToInt(*itr);
          return executeCommandRead(command, address, dumpRawJsonReply);
        } else if (command == "write16" || command == "write32") {
          itr++;
          if (itr == arguments.end()) {
            throw std::runtime_error("missing address argument after write command");
          }
          const u32 address = stringToInt(*itr);
          itr++;
          if (itr == arguments.end()) {
            throw std::runtime_error("missing write-data argument after write command");
          }
          const u32 data = stringToInt(*itr);
          return executeCommandWrite(command, address, data, dumpRawJsonReply);
        } else {
          spdlog::error("Invalid command {}", command);
          return false;
        }
      }();
      if (!commandResult) {
        // If any command fails, return early.
        return false;
      }
      itr++;
    }
    return true;
  }

  static constexpr u16 TCPPortNumber = 10020;
  static constexpr u32 TimeOutInMilisecond = 1000;  // 1 sec

 private:
  u32 stringToInt(const std::string& str) const {
    if (str.size() > 2 && str.at(0) == '0' && str.at(1) == 'x') {
      return std::stoi(str, nullptr, 16);
    } else {
      return std::stoi(str);
    }
  }

  void sendCommand(const picojson::object& object) {
    const std::string jsonString = picojson::value(object).serialize();
    socket_.send(jsonString.c_str(), jsonString.size());
    spdlog::info("Command message sent to growth_daq: {}", jsonString);
  }

  std::optional<picojson::object> receiveReply() {
    zmq::message_t replyMessage{};
    //  Wait for next request from client
    if (socket_.recv(&replyMessage)) {
      spdlog::info("Reply message received from growth_daq");
    } else {
      if (errno != EAGAIN) {
        spdlog::error("Failed to receive a reply from growth_daq");
      } else {
        spdlog::error("Receive reply timeout");
      }
      return {};
    }

    const std::string replyMessageString{static_cast<char*>(replyMessage.data()), replyMessage.size()};
    spdlog::info("{}", replyMessageString);

    picojson::value v;
    picojson::parse(v, replyMessageString);

    if (v.is<picojson::object>()) {
      return v.get<picojson::object>();
    } else {
      spdlog::error("Replied message was not a valid JSON object");
      return {};
    }
  }

  zmq::context_t context_{};
  zmq::socket_t socket_;
  zmq::message_t replyMessage_{};
};

void showImplementedCommands() {
  using ArgumentList = std::vector<std::string>;
  using CommandEntry = std::tuple<std::string, ArgumentList, std::string>;
  const std::vector<CommandEntry> commandList{
      CommandEntry{"read16", {"ADDRESS"}, "Read 16-bit word"},            //
      CommandEntry{"read32", {"ADDRESS"}, "Read 32-bit word"},            //
      CommandEntry{"write16", {"ADDRESS", "DATA"}, "Write 32-bit word"},  //
      CommandEntry{"write32", {"ADDRESS", "DATA"}, "Write 32-bit word"},  //
  };

  std::cout << "Available commands:" << '\n';
  for (const auto [commandName, argumentList, helpMessage] : commandList) {
    std::cout << fmt::format("{:<10s} {:<40s} {}", commandName, stringutil::join(argumentList, " "), helpMessage)
              << '\n';
  }
}

void showHelp(const cxxopts::Options& options) {
  std::cout << options.help() << '\n';
  showImplementedCommands();
  exit(0);
}

int main(int argc, char* argv[]) {
  cxxopts::Options options("growth_cli", "Command-line interface to control growth_daq");
  options.add_options()  //
      ("host", "Hostname where growth_daq is running (default = localhost)",
       cxxopts::value<std::string>()->default_value("localhost"))                         //
      ("r,raw", "Dump raw JSON message", cxxopts::value<bool>()->default_value("false"))  //
      ("v,verbose", "Verbose output", cxxopts::value<bool>()->default_value("false"))     //
      ("h,help", "Show help message", cxxopts::value<bool>()->default_value("false"));
  options.positional_help("[command0 arg0-a arg0-b ... command1 arg1-a arg1-b ...]").show_positional_help();

  auto result = options.parse(argc, argv);
  if (result["verbose"].as<bool>()) {
    spdlog::set_level(spdlog::level::level_enum::info);
  } else {
    spdlog::set_level(spdlog::level::level_enum::warn);
  }
  if (result["help"].as<bool>()) {
    showHelp(options);
  }
  const auto& hostname = result["host"].as<std::string>();
  const auto& dumpRawJsonReply = result["raw"].as<bool>();
  const auto& positional = result.unmatched();

  MessageClient client(hostname);
  return client.parseAndExecute(positional, dumpRawJsonReply) ? 0 : 1;
}
