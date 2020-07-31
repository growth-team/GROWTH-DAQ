#ifndef SRC_MESSAGESERVER_HH_
#define SRC_MESSAGESERVER_HH_

#include "types.h"
#include "CxxUtilities/CxxUtilities.hh"
#include "picojson.h"

#include <iomanip>
#include <sstream>
#include <memory>

// ZeroMQ
#include <unistd.h>
#include <string>
#include <zmq.hpp>

class MainThread;

/** Receives message from a client, and process the message.
 * Typical messages include:
 * <ul>
 *   <li> {"command": "stop"}  => stop the target thread </li>
 * </ul>
 */
class MessageServer : public CxxUtilities::StoppableThread {
 public:
  /** @param[in] mainThread a pointer of Thread instance which will be controlled by this thread
   */
  MessageServer(std::shared_ptr<MainThread> mainThread);
  ~MessageServer();
  void run();
  static constexpr u16 TCPPortNumber  = 10020;
  static constexpr int TimeOutInMilisecond = 1000;  // 1 sec

 private:
  picojson::object processMessage(const std::string& messagePayload);
  picojson::object processPingCommand();
  picojson::object processStopCommand();
  picojson::object processPauseCommand();
  picojson::object processResumeCommand();
  picojson::object processGetStatusCommand();
  picojson::object processStartNewOutputFileCommand();

  zmq::context_t context_{};
  zmq::socket_t socket_;
  zmq::message_t replyMessage_{};
  std::shared_ptr<MainThread> mainThread_{};
};

#endif /* SRC_MESSAGESERVER_HH_ */
