#include <iostream>

#include "MainThread.hh"
#include "MessageServer.hh"

MessageServer::MessageServer(std::shared_ptr<MainThread> mainThread)
    :  //
      context_(1),
      socket_(context_, ZMQ_REP),
      mainThread_(mainThread) {
  std::stringstream ss{};
  ss << "tcp://*:" << TCPPortNumber;
  const int timeout = TimeOutInMilisecond;
  socket_.setsockopt(ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
  socket_.bind(ss.str().c_str());
  // Show message
  using namespace std;
  cout << "MessageServer has started to accept IPC commands." << endl;
}

MessageServer::~MessageServer() {}

void MessageServer::run() {
  using namespace std;
  while (!stopped) {
    zmq::message_t request;
    //  Wait for next request from client
    if (socket_.recv(&request)) {
#ifdef DEBUG_MESSAGESERVER
      std::cout << "MessageServer: Received " << std::endl;
#endif
    } else {
      if (errno != EAGAIN) {
        cerr << "Error: MessageServer::run(): receive failed." << endl;
        ::exit(-1);
      } else {
#ifdef DEBUG_MESSAGESERVER
        cout << "MessageServer::run(): timed out. continue." << endl;
#endif
        continue;
      }
    }

    // Construct a string from received data
    std::string messagePayload{};
    messagePayload.assign(static_cast<char*>(request.data()), request.size());
#ifdef DEBUG_MESSAGESERVER
    cout << "MessageServer::run(): received message" << endl;
    cout << messagePayload << endl;
    cout << endl;
#endif

    // Process received message
    picojson::object replyJSON = processMessage(messagePayload);

    // Reply
    std::string replyMessageString = picojson::value(replyJSON).serialize();
    socket_.send(replyMessageString.c_str(), replyMessageString.size());
  }
}

picojson::object MessageServer::processMessage(const std::string& messagePayload) {
  using namespace std;
  picojson::value v;
  picojson::parse(v, messagePayload);
  if (v.is<picojson::object>()) {
#ifdef DEBUG_MESSAGESERVER
    cout << "MessageServer::processMessage(): received object is: {" << endl;
#endif
    for (auto& it : v.get<picojson::object>()) {
#ifdef DEBUG_MESSAGESERVER
      cout << it.first << ": " << it.second.to_str() << endl;
#endif
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
        }
      }
    }
#ifdef DEBUG_MESSAGESERVER
    cout << "}";
#endif
  }
  // Return error message if the received command is invalid
  picojson::object errorMessage;
  errorMessage["status"] = picojson::value("error");
  errorMessage["unixTime"] = picojson::value(static_cast<f64>(CxxUtilities::Time::getUNIXTimeAsUInt32()));
  errorMessage["message"] = picojson::value("invalid command");
  return errorMessage;
}

picojson::object MessageServer::processPingCommand() {
#ifdef DEBUG_MESSAGESERVER
  cout << "MessageServer::processMessage(): ping command received." << endl;
#endif
  // Construct reply message
  picojson::object replyMessage;
  replyMessage["status"] = picojson::value("ok");
  replyMessage["unixTime"] = picojson::value(static_cast<f64>(CxxUtilities::Time::getUNIXTimeAsUInt32()));
  return replyMessage;
}

picojson::object MessageServer::processStopCommand() {
#ifdef DEBUG_MESSAGESERVER
  cout << "MessageServer::processMessage(): stop command received." << endl;
#endif
  // Stop target thread and self
  if (mainThread_ != nullptr) {
    mainThread_->stop();
  }
  this->stop();
  // Construct reply message
  picojson::object replyMessage;
  replyMessage["status"] = picojson::value("ok");
  replyMessage["unixTime"] = picojson::value(static_cast<f64>(CxxUtilities::Time::getUNIXTimeAsUInt32()));
  return replyMessage;
}

picojson::object MessageServer::processPauseCommand() {
#ifdef DEBUG_MESSAGESERVER
  cout << "MessageServer::processMessage(): pause command received." << endl;
#endif
  // Stop target thread and self
  if (mainThread_ != nullptr) {
    mainThread_->stop();
  }
  // Construct reply message
  picojson::object replyMessage;
  replyMessage["status"] = picojson::value("ok");
  replyMessage["unixTime"] = picojson::value(static_cast<f64>(CxxUtilities::Time::getUNIXTimeAsUInt32()));
  return replyMessage;
}

picojson::object MessageServer::processResumeCommand() {
#ifdef DEBUG_MESSAGESERVER
  cout << "MessageServer::processMessage(): resume command received." << endl;
#endif
  // Stop target thread and self
  if (mainThread_ != nullptr) {
    if (mainThread_->getDAQStatus() == DAQStatus::Paused) {
      mainThread_->start();
    }
  }
  // Construct reply message
  picojson::object replyMessage;
  replyMessage["status"] = picojson::value("ok");
  replyMessage["unixTime"] = picojson::value(static_cast<f64>(CxxUtilities::Time::getUNIXTimeAsUInt32()));
  return replyMessage;
}

picojson::object MessageServer::processGetStatusCommand() {
#ifdef DEBUG_MESSAGESERVER
  cout << "MessageServer::processMessage(): getStatus command received." << endl;
#endif
  // Construct reply message
  picojson::object replyMessage;
  replyMessage["status"] = picojson::value("ok");
  replyMessage["unixTime"] = picojson::value(static_cast<f64>(CxxUtilities::Time::getUNIXTimeAsUInt32()));
  if (mainThread_->getDAQStatus() == DAQStatus::Running) {
    replyMessage["daqStatus"] = picojson::value("Running");
  } else {
    replyMessage["daqStatus"] = picojson::value("Paused");
  }
  replyMessage["outputFileName"] = picojson::value(mainThread_->getOutputFileName());
  replyMessage["elapsedTime"] = picojson::value(static_cast<f64>(mainThread_->getElapsedTime()));
  replyMessage["nEvents"] = picojson::value(static_cast<f64>(mainThread_->getNEvents()));
  replyMessage["elapsedTimeOfCurrentOutputFile"] =  //
      picojson::value(static_cast<f64>(mainThread_->getElapsedTimeOfCurrentOutputFile()));
  replyMessage["nEventsOfCurrentOutputFile"] =  //
      picojson::value(static_cast<f64>(mainThread_->getNEventsOfCurrentOutputFile()));
  return replyMessage;
}

picojson::object MessageServer::processStartNewOutputFileCommand() {
  // Switch output file to a new one
  mainThread_->startNewOutputFile();

  // Construct reply message
  picojson::object replyMessage;
  replyMessage["status"] = picojson::value("ok");
  replyMessage["unixTime"] = picojson::value(static_cast<f64>(CxxUtilities::Time::getUNIXTimeAsUInt32()));
  return replyMessage;
}
