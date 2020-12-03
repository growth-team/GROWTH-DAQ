#ifndef SPACEWIRE_RMAPTRANSACTION_HH_
#define SPACEWIRE_RMAPTRANSACTION_HH_

#include <mutex>

#include "CxxUtilities/CxxUtilities.hh"
#include "rmappacket.hh"
#include "spacewire/types.hh"

class RMAPTransaction {
 public:
  static constexpr f64 DefaultTimeoutDuration = 1000.0;
  enum State {
    // for RMAPInitiator-related transaction
    NotInitiated = 0x00,
    Initiated = 0x01,
    CommandSent = 0x02,
    ReplyReceived = 0x03,
    Timeout = 0x04,
    // for RMAPTarget-related transaction
    CommandPacketReceived = 0x10,
    ReplySet = 0x11,
    ReplySent = 0x12,
    Aborted = 0x13,
    ReplyCompleted = 0x14
  };

  u8 targetLogicalAddress{};
  u8 initiatorLogicalAddress{};
  u16 transactionID{};
  double timeoutDuration = DefaultTimeoutDuration;
  State state{};
  std::mutex stateMutex;
  RMAPPacket* commandPacket{};
  RMAPPacket* replyPacket{};


  void signal() {

  }
  void wait(u32 millis=1000) {
	  std::unique_lock<std::mutex> conditionLock(conditionMutex);
	  condition.wait_for(conditionLock, __d)

  }

  void setStateWithLock(State newState) {
    std::lock_guard<std::mutex> guard(stateMutex);
    state = newState;
  }

 private:
  std::mutex conditionMutex;
  std::unique_lock<std::mutex> conditionLock;
  std::condition_variable condition;

};

#endif
