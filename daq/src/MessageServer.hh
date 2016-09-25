#ifndef SRC_MESSAGESERVER_HH_
#define SRC_MESSAGESERVER_HH_

#include <iomanip>
#include <sstream>
#include "CxxUtilities/CxxUtilities.hh"

// ZeroMQ
#include <zmq.hpp>
#include <string>
#include <iostream>
#include <unistd.h>

// picojson
#include "picojson.h"

#include "MainThread.hh"

#define DEBUG_MESSAGESERVER

#ifdef DEBUG_MESSAGESERVER
using namespace std;
#endif

/** Receives message from a client, and process the message.
 * Typical messages include:
 * <ul>
 *   <li> {"command": "stop"}  => stop the target thread </li>
 * </ul>
 */
class MessageServer: public CxxUtilities::StoppableThread {
public:
	/** @param[in] mainThread a pointer of Thread instance which will be controlled by this thread
	 */
	MessageServer(MainThread* mainThread) : //
			context(1), socket(context, ZMQ_REP), mainThread(mainThread) {
		std::stringstream ss;
		ss << "tcp://*:" << TCPPortNumber;
		int timeout = TimeOutInMilisecond;
		socket.setsockopt(ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
		socket.bind(ss.str().c_str());
		// Show message
		using namespace std;
		cout << "MessageServer has started to accept IPC commands." << endl;
	}

public:
	~MessageServer() {
	}

public:
	void run() {
		using namespace std;
		while (!stopped) {
			zmq::message_t request;
			//  Wait for next request from client
			if (socket.recv(&request)) {
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
			std::string messagePayload;
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
			socket.send(replyMessageString.c_str(), replyMessageString.size());
		}
	}

private:
	picojson::object processMessage(const std::string& messagePayload) {
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
		errorMessage["unixTime"] = picojson::value(static_cast<double>(CxxUtilities::Time::getUNIXTimeAsUInt32()));
		errorMessage["message"] = picojson::value("invalid command");
		return errorMessage;
	}

public:
	static const uint16_t TCPPortNumber = 10020;
	static const int TimeOutInMilisecond = 1000; // 1 sec

private:
	picojson::object processPingCommand() {
#ifdef DEBUG_MESSAGESERVER
		cout << "MessageServer::processMessage(): ping command received." << endl;
#endif
		// Construct reply message
		picojson::object replyMessage;
		replyMessage["status"] = picojson::value("ok");
		replyMessage["unixTime"] = picojson::value(static_cast<double>(CxxUtilities::Time::getUNIXTimeAsUInt32()));
		return replyMessage;
	}

private:
	picojson::object processStopCommand() {
#ifdef DEBUG_MESSAGESERVER
		cout << "MessageServer::processMessage(): stop command received." << endl;
#endif
		// Stop target thread and self
		if (mainThread != nullptr) {
			mainThread->stop();
		}
		this->stop();
		// Construct reply message
		picojson::object replyMessage;
		replyMessage["status"] = picojson::value("ok");
		replyMessage["unixTime"] = picojson::value(static_cast<double>(CxxUtilities::Time::getUNIXTimeAsUInt32()));
		return replyMessage;
	}

	picojson::object processPauseCommand() {
#ifdef DEBUG_MESSAGESERVER
		cout << "MessageServer::processMessage(): pause command received." << endl;
#endif
		// Stop target thread and self
		if (mainThread != nullptr) {
			mainThread->stop();
		}
		// Construct reply message
		picojson::object replyMessage;
		replyMessage["status"] = picojson::value("ok");
		replyMessage["unixTime"] = picojson::value(static_cast<double>(CxxUtilities::Time::getUNIXTimeAsUInt32()));
		return replyMessage;
	}

	picojson::object processResumeCommand() {
#ifdef DEBUG_MESSAGESERVER
		cout << "MessageServer::processMessage(): resume command received." << endl;
#endif
		// Stop target thread and self
		if (mainThread != nullptr) {
			if (mainThread->getDAQStatus() == DAQStatus::Paused) {
				mainThread->start();
			}
		}
		// Construct reply message
		picojson::object replyMessage;
		replyMessage["status"] = picojson::value("ok");
		replyMessage["unixTime"] = picojson::value(static_cast<double>(CxxUtilities::Time::getUNIXTimeAsUInt32()));
		return replyMessage;
	}

private:
	picojson::object processGetStatusCommand() {
#ifdef DEBUG_MESSAGESERVER
		cout << "MessageServer::processMessage(): getStatus command received." << endl;
#endif
		// Construct reply message
		picojson::object replyMessage;
		replyMessage["status"] = picojson::value("ok");
		replyMessage["unixTime"] = picojson::value(static_cast<double>(CxxUtilities::Time::getUNIXTimeAsUInt32()));
		if(mainThread->getDAQStatus()==DAQStatus::Running){
			replyMessage["daqStatus"] = picojson::value("Running");
		}else{
			replyMessage["daqStatus"] = picojson::value("Paused");
		}
		replyMessage["outputFileName"] = picojson::value(mainThread->getOutputFileName());
		replyMessage["elapsedTime"] = picojson::value(static_cast<double>(mainThread->getElapsedTime()));
		replyMessage["nEvents"] = picojson::value(static_cast<double>(mainThread->getNEvents()));
		replyMessage["elapsedTimeOfCurrentOutputFile"] = //
				picojson::value(static_cast<double>(mainThread->getElapsedTimeOfCurrentOutputFile()));
		replyMessage["nEventsOfCurrentOutputFile"] = //
				picojson::value(static_cast<double>(mainThread->getNEventsOfCurrentOutputFile()));
		return replyMessage;
	}

private:
	picojson::object processStartNewOutputFileCommand() {
		// Switch output file to a new one
		mainThread->startNewOutputFile();

		// Construct reply message
		picojson::object replyMessage;
		replyMessage["status"] = picojson::value("ok");
		replyMessage["unixTime"] = picojson::value(static_cast<double>(CxxUtilities::Time::getUNIXTimeAsUInt32()));
		return replyMessage;
	}

private:
	zmq::context_t context;
	zmq::socket_t socket;
	zmq::message_t replyMessage;

private:
	MainThread* mainThread;
};

#endif /* SRC_MESSAGESERVER_HH_ */
