#ifndef SPACEWIRE_RMAPINITIATOR_HH_
#define SPACEWIRE_RMAPINITIATOR_HH_

#include <chrono>
#include <condition_variable>
#include <mutex>

#include "spacewire/rmapengine.hh"
#include "spacewire/rmappacket.hh"
#include "spacewire/rmapprotocol.hh"
#include "spacewire/rmapreplystatus.hh"
#include "spacewire/rmaptargetnode.hh"

class RMAPInitiatorException : public Exception {
 public:
  enum {
    Timeout = 0x100,
    Aborted = 0x200,
    ReadReplyWithInsufficientData,
    ReadReplyWithTooMuchData,
    UnexpectedWriteReplyReceived,
    RMAPTransactionCouldNotBeInitiated,
  };

  RMAPInitiatorException(u32 status) : Exception(status) {}
  virtual ~RMAPInitiatorException() = default;
  std::string toString() const override {
    std::string result;
    switch (status_) {
      case Timeout:
        result = "Timeout";
        break;
      case Aborted:
        result = "Aborted";
        break;
      case ReadReplyWithInsufficientData:
        result = "ReadReplyWithInsufficientData";
        break;
      case ReadReplyWithTooMuchData:
        result = "ReadReplyWithTooMuchData";
        break;
      case UnexpectedWriteReplyReceived:
        result = "UnexpectedWriteReplyReceived";
        break;
      case RMAPTransactionCouldNotBeInitiated:
        result = "RMAPTransactionCouldNotBeInitiated";
        break;
      default:
        result = "Undefined status";
        break;
    }
    return result;
  }
};

class RMAPInitiator {
 public:
  RMAPInitiator(RMAPEnginePtr rmapEngine) : rmapEngine_(rmapEngine) {
    commandPacket.reset(new RMAPPacket());
    replyPacket.reset(new RMAPPacket());
  }

  void read(const RMAPTargetNode* rmapTargetNode, u32 memoryAddress, u32 length, u8* buffer,
            i32 timeoutDurationMillisec = DefaultTimeoutDurationMillisec) {
    commandPacket->setInitiatorLogicalAddress(this->getInitiatorLogicalAddress());
    commandPacket->setRead();
    commandPacket->setCommand();
    commandPacket->setIncrementFlag(incrementMode);
    commandPacket->setVerifyFlag(false);
    commandPacket->setReplyFlag(true);
    commandPacket->setExtendedAddress(0x00);
    commandPacket->setAddress(memoryAddress);
    commandPacket->setDataLength(length);
    commandPacket->clearData();
    // InitiatorLogicalAddress might be updated in commandPacket->setRMAPTargetInformation(rmapTargetNode) below.
    commandPacket->setRMAPTargetInformation(rmapTargetNode);

    if (replyPacket) {
      rmapEngine_->putBackRMAPPacketInstnce(std::move(replyPacket));
    }

    std::unique_lock<std::mutex> lock(replyWaitMutex_);
    const TransactionID assignedTransactionID = rmapEngine_->initiateTransaction(commandPacket.get(), this);
    const auto rel_time = std::chrono::milliseconds(timeoutDurationMillisec);
    replyWaitCondition_.wait_for(lock, rel_time, [&]() { return static_cast<bool>(replyPacket); });

    if (replyPacket) {
      if (replyPacket->getStatus() != RMAPReplyStatus::CommandExcecutedSuccessfully) {
        throw RMAPReplyException(replyPacket->getStatus());
      }
      if (length < replyPacket->getDataBuffer()->size()) {
        throw RMAPInitiatorException(RMAPInitiatorException::ReadReplyWithInsufficientData);
      }
      replyPacket->getData(buffer, length);
      // when successful, replay packet is retained until next transaction for inspection by user application
      return;
    } else {
      // cancel transaction (return transaction ID)
      rmapEngine_->cancelTransaction(assignedTransactionID);
      throw RMAPInitiatorException(RMAPInitiatorException::Timeout);
    }
  }

  void write(const RMAPTargetNode* rmapTargetNode, u32 memoryAddress, const u8* data, u32 length,
             i32 timeoutDurationMillisec = DefaultTimeoutDurationMillisec) {
    commandPacket->setInitiatorLogicalAddress(this->getInitiatorLogicalAddress());
    commandPacket->setWrite();
    commandPacket->setCommand();
    commandPacket->setIncrementFlag(incrementMode);
    commandPacket->setVerifyFlag(verifyMode);
    commandPacket->setReplyFlag(replyMode);
    commandPacket->setExtendedAddress(0x00);
    commandPacket->setAddress(memoryAddress);
    commandPacket->setDataLength(length);
    commandPacket->setRMAPTargetInformation(rmapTargetNode);
    commandPacket->setData(data, length);

    if (replyPacket) {
      rmapEngine_->putBackRMAPPacketInstnce(std::move(replyPacket));
    }

    std::unique_lock<std::mutex> lock(replyWaitMutex_);
    const TransactionID assignedTransactionID = rmapEngine_->initiateTransaction(commandPacket.get(), this);

    if (!replyMode) {  // if reply is not expected
      return;
    }
    const auto rel_time = std::chrono::milliseconds(timeoutDurationMillisec);
    replyWaitCondition_.wait_for(lock, rel_time, [&]() { return static_cast<bool>(replyPacket); });
    if (replyPacket) {
      if (replyPacket->getStatus() == RMAPReplyStatus::CommandExcecutedSuccessfully) {
        return;
      } else {
        throw RMAPReplyException(replyPacket->getStatus());
      }
    } else {
      rmapEngine_->cancelTransaction(assignedTransactionID);
      throw RMAPInitiatorException(RMAPInitiatorException::Timeout);
    }
  }

  void replyReceived(RMAPPacketPtr&& packet) {
    replyPacket = std::move(packet);
    std::lock_guard<std::mutex> guard(replyWaitMutex_);
    replyWaitCondition_.notify_one();
  }

  u8 getInitiatorLogicalAddress() const { return initiatorLogicalAddress; }
  bool getReplyMode() const { return replyMode; }
  bool getIncrementMode() const { return incrementMode; }
  bool getVerifyMode() const { return verifyMode; }

  void setInitiatorLogicalAddress(u8 logicalAddress) { initiatorLogicalAddress = logicalAddress; }
  void setReplyMode(bool replyMode) { this->replyMode = replyMode; }
  void setIncrementMode(bool incrementMode) { this->incrementMode = incrementMode; }
  void setVerifyMode(bool verifyMode) { this->verifyMode = verifyMode; }

  static constexpr i32 DefaultTimeoutDurationMillisec = 1000;
  static const bool DefaultIncrementMode = true;
  static const bool DefaultVerifyMode = true;
  static const bool DefaultReplyMode = true;

 private:
  RMAPEnginePtr rmapEngine_;
  RMAPPacketPtr commandPacket;
  RMAPPacketPtr replyPacket;

  u8 initiatorLogicalAddress = SpaceWireProtocol::DefaultLogicalAddress;
  bool incrementMode = DefaultIncrementMode;
  bool verifyMode = DefaultVerifyMode;
  bool replyMode = DefaultReplyMode;

  std::condition_variable replyWaitCondition_;
  std::mutex replyWaitMutex_;
};
#endif
