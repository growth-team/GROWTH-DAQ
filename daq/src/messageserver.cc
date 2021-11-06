#include "messageserver.hh"
#include "mainthread.hh"

#include <iostream>
#include "spdlog/spdlog.h"

MessageServer::MessageServer(std::shared_ptr<MainThread> mainThread)
    :  //
      context_(1),
      socket_(context_, ZMQ_REP),
      mainThread_(mainThread) {}

MessageServer::~MessageServer() = default;

void MessageServer::run() {
  std::stringstream ss{};
  ss << "tcp://*:" << TCPPortNumber;
  const int timeout = TimeOutInMilisecond;
  socket_.setsockopt(ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
  socket_.bind(ss.str().c_str());
  spdlog::info("MessageServer has started to accept IPC commands.");

  while (!stopped_) {
    zmq::message_t request{};
    //  Wait for next request from client
    if (socket_.recv(&request)) {
      spdlog::info("MessageServer: A request is received");
    } else {
      if (errno != EAGAIN) {
        spdlog::error("MessageServer::run(): receive failed");
        ::exit(-1);
      } else {
        continue;
      }
    }

    // Construct a string from received data
    std::string messagePayload{};
    messagePayload.assign(static_cast<char*>(request.data()), request.size());
    spdlog::info("MessageServer::run(): received message '{}", messagePayload);

    // Process received message
    const auto replyJSON = processMessage(messagePayload);

    // Reply
    std::string replyMessageString = picojson::value(replyJSON).serialize();
    socket_.send(replyMessageString.c_str(), replyMessageString.size());
  }
}

picojson::object MessageServer::processMessage(const std::string& messagePayload) {
  picojson::value v;
  picojson::parse(v, messagePayload);
  if (v.is<picojson::object>()) {
    for (auto& it : v.get<picojson::object>()) {
      if (it.first == "command") {
        if (it.second.to_str() == "ping") {
          return processPingCommand();
        } else if (it.second.to_str() == "stop") {
          return processStopCommand();
        } else if (it.second.to_str() == "pause") {
          return processPauseCommand();
        } else if (it.second.to_str() == "resume") {
          return processResumeCommand();
        } else if (it.second.to_str() == "getStatus") {
          return processGetStatusCommand();
        } else if (it.second.to_str() == "startNewOutputFile") {
          return processStartNewOutputFileCommand();
        } else if (it.second.to_str() == "registerRead" || it.second.to_str() == "registerWrite") {
          return processRegisterReadWrite(v.get<picojson::object>());
        }
      }
    }
  }
  // Return error message if the received command is invalid
  picojson::object errorMessage{};
  errorMessage["status"] = picojson::value("error");
  errorMessage["unixTime"] = picojson::value(static_cast<f64>(timeutil::getUNIXTimeAsUInt64()));
  errorMessage["message"] = picojson::value("invalid command");
  return errorMessage;
}

picojson::object MessageServer::processPingCommand() {
  spdlog::info("MessageServer: ping command received");
  // Construct reply message
  picojson::object replyMessage{};
  replyMessage["status"] = picojson::value("ok");
  replyMessage["unixTime"] = picojson::value(static_cast<f64>(timeutil::getUNIXTimeAsUInt64()));
  return replyMessage;
}

picojson::object MessageServer::processStopCommand() {
  spdlog::info("MessageServer: stop command received");
  // Stop target thread and self
  if (mainThread_) {
    mainThread_->stop();
  }
  this->stop();
  // Construct reply message
  picojson::object replyMessage{};
  replyMessage["status"] = picojson::value("ok");
  replyMessage["unixTime"] = picojson::value(static_cast<f64>(timeutil::getUNIXTimeAsUInt64()));
  return replyMessage;
}

picojson::object MessageServer::processPauseCommand() {
  spdlog::info("MessageServer: pause command received");
  // Stop target thread and self
  if (mainThread_) {
    mainThread_->stop();
  }
  // Construct reply message
  picojson::object replyMessage;
  replyMessage["status"] = picojson::value("ok");
  replyMessage["unixTime"] = picojson::value(static_cast<f64>(timeutil::getUNIXTimeAsUInt64()));
  return replyMessage;
}

picojson::object MessageServer::processResumeCommand() {
  spdlog::info("MessageServer: resume command received");

  if (mainThread_) {
    if (mainThread_->getDAQStatus() == DAQStatus::Paused) {
      mainThread_->start();
    }
  }
  // Construct reply message
  picojson::object replyMessage{};
  replyMessage["status"] = picojson::value("ok");
  replyMessage["unixTime"] = picojson::value(static_cast<f64>(timeutil::getUNIXTimeAsUInt64()));
  return replyMessage;
}

picojson::object MessageServer::processGetStatusCommand() {
  spdlog::info("MessageServer: getStatus command received");

  // Construct reply message
  picojson::object replyMessage{};
  replyMessage["status"] = picojson::value("ok");
  replyMessage["unixTime"] = picojson::value(static_cast<f64>(timeutil::getUNIXTimeAsUInt64()));
  if (mainThread_->getDAQStatus() == DAQStatus::Running) {
    replyMessage["daqStatus"] = picojson::value("Running");
  } else {
    replyMessage["daqStatus"] = picojson::value("Paused");
  }
  replyMessage["outputFileName"] = picojson::value(mainThread_->getOutputFileName());
  replyMessage["elapsedTime"] = picojson::value(static_cast<f64>(mainThread_->getElapsedTime().count()));
  replyMessage["nEvents"] = picojson::value(static_cast<f64>(mainThread_->getNEvents()));
  replyMessage["elapsedTimeOfCurrentOutputFile"] =  //
      picojson::value(static_cast<f64>(mainThread_->getElapsedTimeOfCurrentOutputFile().count()));
  replyMessage["nEventsOfCurrentOutputFile"] =  //
      picojson::value(static_cast<f64>(mainThread_->getNEventsOfCurrentOutputFile()));
  return replyMessage;
}

picojson::object MessageServer::processStartNewOutputFileCommand() {
  spdlog::info("MessageServer: startNewOutputFile command received");

  // Switch output file to a new one
  mainThread_->startNewOutputFile();

  // Construct reply message
  picojson::object replyMessage{};
  replyMessage["status"] = picojson::value("ok");
  replyMessage["unixTime"] = picojson::value(static_cast<f64>(timeutil::getUNIXTimeAsUInt64()));
  return replyMessage;
}

picojson::object MessageServer::processRegisterReadWrite(const picojson::object& command) {
  const std::string& commandStr = command.at("command").to_str();
  spdlog::info("MessageServer: {} command received", commandStr);

  picojson::object replyMessage{};
  replyMessage["unixTime"] = picojson::value(static_cast<f64>(timeutil::getUNIXTimeAsUInt64()));
  replyMessage["status"] = picojson::value("error");

  if (command.count("address") == 0) {
    replyMessage["message"] = picojson::value("'address' field is missing");
    return replyMessage;
  }
  if (!command.at("address").is<f64>()) {
    replyMessage["message"] = picojson::value("'address' field must be a number");
    return replyMessage;
  }
  const u32 address = static_cast<u32>(command.at("address").get<f64>());
  replyMessage["registerAddress"] = picojson::value(static_cast<f64>(address));
  if (commandStr == "registerWrite") {
    if (command.count("value") == 0 || !command.at("value").is<f64>()) {
      replyMessage["message"] =
          picojson::value("write value must be specified as a number for a registerWrite command");
      return replyMessage;
    }
    const u16 value = static_cast<u16>(command.at("value").get<f64>());
    try {
      const auto writeResult = mainThread_->writeRegister16(address, value);
      if (writeResult) {
        replyMessage["status"] = picojson::value("ok");
      }
    } catch (...) {
      spdlog::error("Failed to write address {:08x}", address);
      replyMessage["message"] = picojson::value("register write access failed");
    }
  } else {
    try {
      const auto readResult = mainThread_->readRegister16(address);
      if (readResult) {
        replyMessage["status"] = picojson::value("ok");
        replyMessage["registerValue"] = picojson::value(static_cast<f64>(readResult.value()));
      }
    } catch (...) {
      spdlog::error("Failed to read address {:08x}", address);
      replyMessage["message"] = picojson::value("register read access failed");
    }
  }
  return replyMessage;
}
