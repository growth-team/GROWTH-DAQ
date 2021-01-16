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

#include "spdlog/spdlog.h"

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

  explicit RMAPInitiatorException(u32 status) : Exception(status) {}
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
  explicit RMAPInitiator(RMAPEnginePtr rmapEngine) : rmapEngine_(std::move(rmapEngine)) {
    commandPacket_ = std::make_unique<RMAPPacket>();
    replyPacket_ = std::make_unique<RMAPPacket>();
  }

  void read(const RMAPTargetNode* rmapTargetNode, u32 memoryAddress, u32 length, u8* buffer,
            i32 timeoutDurationMillisec = DDEFAULT_TIMEOUT_DURATION_MILLISEC) {
    commandPacket_->setInitiatorLogicalAddress(getInitiatorLogicalAddress());
    commandPacket_->setRead();
    commandPacket_->setCommand();
    commandPacket_->setIncrementFlag(incrementMode_);
    commandPacket_->setVerifyFlag(false);
    commandPacket_->setReplyFlag(true);
    commandPacket_->setExtendedAddress(0x00);
    commandPacket_->setAddress(memoryAddress);
    commandPacket_->setDataLength(length);
    commandPacket_->clearData();
    // InitiatorLogicalAddress might be updated in commandPacket->setRMAPTargetInformation(rmapTargetNode) below.
    commandPacket_->setRMAPTargetInformation(rmapTargetNode);

    if (replyPacket_) {
      rmapEngine_->putBackRMAPPacketInstnce(std::move(replyPacket_));
      replyPacketSet_=false;
    }

    std::unique_lock<std::mutex> lock(replyWaitMutex_);

    const TransactionID assignedTransactionID = rmapEngine_->initiateTransaction(commandPacket_.get(), this);

    const auto rel_time = std::chrono::milliseconds(timeoutDurationMillisec);
    replyWaitCondition_.wait_for(lock, rel_time, [&]() { return static_cast<bool>(replyPacket_); });

    if (replyPacketSet_ && replyPacket_) {
      if (replyPacket_->getStatus() != RMAPReplyStatus::CommandExcecutedSuccessfully) {
        throw RMAPReplyException(replyPacket_->getStatus());
      }
      if (length < replyPacket_->getDataBuffer()->size()) {
        throw RMAPInitiatorException(RMAPInitiatorException::ReadReplyWithInsufficientData);
      }
      replyPacket_->getData(buffer, length);
      // when successful, replay packet is retained until next transaction for inspection by user application
      return;
    } else {
      // cancel transaction (return transaction ID)
      rmapEngine_->cancelTransaction(assignedTransactionID);
      throw RMAPInitiatorException(RMAPInitiatorException::Timeout);
    }
  }

  void write(const RMAPTargetNode* rmapTargetNode, u32 memoryAddress, const u8* data, u32 length,
             i32 timeoutDurationMillisec = DDEFAULT_TIMEOUT_DURATION_MILLISEC) {
    commandPacket_->setInitiatorLogicalAddress(getInitiatorLogicalAddress());
    commandPacket_->setWrite();
    commandPacket_->setCommand();
    commandPacket_->setIncrementFlag(incrementMode_);
    commandPacket_->setVerifyFlag(verifyMode_);
    commandPacket_->setReplyFlag(replyMode_);
    commandPacket_->setExtendedAddress(0x00);
    commandPacket_->setAddress(memoryAddress);
    commandPacket_->setDataLength(length);
    commandPacket_->setRMAPTargetInformation(rmapTargetNode);
    commandPacket_->setData(data, length);

    if (replyPacket_) {
      rmapEngine_->putBackRMAPPacketInstnce(std::move(replyPacket_));
      replyPacketSet_=false;
    }

    std::unique_lock<std::mutex> lock(replyWaitMutex_);
    const TransactionID assignedTransactionID = rmapEngine_->initiateTransaction(commandPacket_.get(), this);

    if (!replyMode_) {  // if reply is not expected
      return;
    }
    const auto rel_time = std::chrono::milliseconds(timeoutDurationMillisec);
    replyWaitCondition_.wait_for(lock, rel_time, [&]() { return static_cast<bool>(replyPacket_); });
    if (replyPacketSet_ && replyPacket_) {
      if (replyPacket_->getStatus() == RMAPReplyStatus::CommandExcecutedSuccessfully) {
        return;
      } else {
        throw RMAPReplyException(replyPacket_->getStatus());
      }
    } else {
      rmapEngine_->cancelTransaction(assignedTransactionID);
      throw RMAPInitiatorException(RMAPInitiatorException::Timeout);
    }
  }

  void replyReceived(RMAPPacketPtr packet) {
    std::lock_guard<std::mutex> guard(replyWaitMutex_);
    replyPacket_ = std::move(packet);
    replyPacketSet_=true;
    replyWaitCondition_.notify_one();
  }

  u8 getInitiatorLogicalAddress() const { return initiatorLogicalAddress_; }
  bool getReplyMode() const { return replyMode_; }
  bool getIncrementMode() const { return incrementMode_; }
  bool getVerifyMode() const { return verifyMode_; }

  void setInitiatorLogicalAddress(u8 logicalAddress) { initiatorLogicalAddress_ = logicalAddress; }
  void setReplyMode(bool replyMode) { replyMode_ = replyMode; }
  void setIncrementMode(bool incrementMode) { incrementMode_ = incrementMode; }
  void setVerifyMode(bool verifyMode) { verifyMode_ = verifyMode; }

  static constexpr i32 DDEFAULT_TIMEOUT_DURATION_MILLISEC = 2000;
  static const bool DEFAULT_INCREMENT_MODE = true;
  static const bool DEFAULT_VERIFY_MODE = true;
  static const bool DEFAULT_REPLY_MODE = true;

 private:
  RMAPEnginePtr rmapEngine_{};
  RMAPPacketPtr commandPacket_{};
  RMAPPacketPtr replyPacket_{};
  std::atomic<bool> replyPacketSet_{false};

  u8 initiatorLogicalAddress_ = SpaceWireProtocol::DEFAULT_LOGICAL_ADDRESS;
  bool incrementMode_ = DEFAULT_INCREMENT_MODE;
  bool verifyMode_ = DEFAULT_VERIFY_MODE;
  bool replyMode_ = DEFAULT_REPLY_MODE;

  std::condition_variable replyWaitCondition_{};
  std::mutex replyWaitMutex_{};
};
#endif
