#ifndef SPACEWIRE_RMAPINITIATOR_HH_
#define SPACEWIRE_RMAPINITIATOR_HH_

#include "spacewire/rmapengine.hh"
#include "spacewire/rmappacket.hh"
#include "spacewire/rmapprotocol.hh"
#include "spacewire/rmapreplystatus.hh"
#include "spacewire/rmaptargetnode.hh"
#include "spacewire/rmaptransaction.hh"

class RMAPInitiatorException : public CxxUtilities::Exception {
 public:
  enum {
    Timeout = 0x100,
    Aborted = 0x200,
    ReadReplyWithInsufficientData,
    ReadReplyWithTooMuchData,
    UnexpectedWriteReplyReceived,
    RMAPTransactionCouldNotBeInitiated,
  };

 public:
  RMAPInitiatorException(u32 status) : CxxUtilities::Exception(status) {}

 public:
  virtual ~RMAPInitiatorException() {}

 public:
  std::string toString() {
    std::string result;
    switch (status) {
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
            f64 timeoutDuration = DefaultTimeoutDuration) {
    std::lock_guard<std::mutex> guard(transactionLock_);
    replyPacket.reset();
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
    /** InitiatorLogicalAddress might be updated in commandPacket->setRMAPTargetInformation(rmapTargetNode) below */
    commandPacket->setRMAPTargetInformation(rmapTargetNode);
    transaction.commandPacket = commandPacket.get();
    try {
      rmapEngine_->initiateTransaction(&transaction);
    } catch (RMAPEngineException& e) {
      transaction.state = RMAPTransaction::NotInitiated;
      throw RMAPInitiatorException(RMAPInitiatorException::RMAPTransactionCouldNotBeInitiated);
    } catch (...) {
      transaction.state = RMAPTransaction::NotInitiated;
      throw RMAPInitiatorException(RMAPInitiatorException::RMAPTransactionCouldNotBeInitiated);
    }
    transaction.condition.wait(timeoutDuration);
    if (transaction.state == RMAPTransaction::ReplyReceived) {
      replyPacket = transaction.replyPacket;
      transaction.replyPacket = nullptr;
      if (replyPacket->getStatus() != RMAPReplyStatus::CommandExcecutedSuccessfully) {
        const u8 replyStatus = replyPacket->getStatus();
        transaction.state = RMAPTransaction::NotInitiated;
        throw RMAPReplyException(replyStatus);
      }
      if (length < replyPacket->getDataBuffer()->size()) {
        transaction.state = RMAPTransaction::NotInitiated;
        throw RMAPInitiatorException(RMAPInitiatorException::ReadReplyWithInsufficientData);
      }
      replyPacket->getData(buffer, length);
      transaction.state = RMAPTransaction::NotInitiated;
      // when successful, replay packet is retained until next transaction for inspection by user application
      return;
    } else {
      // cancel transaction (return transaction ID)
      rmapEngine_->cancelTransaction(&transaction);
      transaction.state = RMAPTransaction::NotInitiated;
      throw RMAPInitiatorException(RMAPInitiatorException::Timeout);
    }
  }

  void write(const RMAPTargetNode* rmapTargetNode, u32 memoryAddress, const u8* data, u32 length,
             f64 timeoutDuration = DefaultTimeoutDuration) {
    std::lock_guard<std::mutex> guard(transactionLock_);
    replyPacket.reset();
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
    transaction.commandPacket = commandPacket.get();
    setRMAPTransactionOptions(transaction);
    rmapEngine_->initiateTransaction(&transaction);

    if (!replyMode) {  // if reply is not expected
      if (transaction.state == RMAPTransaction::Initiated) {
        transaction.state = RMAPTransaction::Initiated;
        return;
      } else {
        transaction.state = RMAPTransaction::NotInitiated;
        // command was not sent successfully
        throw RMAPInitiatorException(RMAPInitiatorException::RMAPTransactionCouldNotBeInitiated);
      }
    }
    transaction.state = RMAPTransaction::CommandSent;

    // if reply is expected
    transaction.condition.wait(timeoutDuration);
    if (transaction.state == RMAPTransaction::CommandSent) {
      if (replyMode) {
        // cancel transaction (return transaction ID)
        rmapEngine_->cancelTransaction(&transaction);
        // reply packet is not created, and therefore the line below is not necessary
        // deleteReplyPacket();
        transaction.state = RMAPTransaction::Initiated;
        throw RMAPInitiatorException(RMAPInitiatorException::Timeout);
      } else {
        transaction.state = RMAPTransaction::NotInitiated;
        return;
      }
    } else if (transaction.state == RMAPTransaction::ReplyReceived) {
      replyPacket = transaction.replyPacket;
      transaction.replyPacket = NULL;
      if (replyPacket->getStatus() != RMAPReplyStatus::CommandExcecutedSuccessfully) {
        u8 replyStatus = replyPacket->getStatus();
        transaction.state = RMAPTransaction::NotInitiated;
        throw RMAPReplyException(replyStatus);
      }
      if (replyPacket->getStatus() == RMAPReplyStatus::CommandExcecutedSuccessfully) {
        // When successful, replay packet is retained until next transaction for inspection by user application
        // deleteReplyPacket();
        transaction.state = RMAPTransaction::NotInitiated;
        return;
      } else {
        u8 replyStatus = replyPacket->getStatus();
        transaction.state = RMAPTransaction::NotInitiated;
        throw RMAPReplyException(replyStatus);
      }
    } else if (transaction.state == RMAPTransaction::Timeout) {
      // cancel transaction (return transaction ID)
      rmapEngine_->cancelTransaction(&transaction);
      transaction.state = RMAPTransaction::NotInitiated;
      throw RMAPInitiatorException(RMAPInitiatorException::Timeout);
    }
  }

  u8 getInitiatorLogicalAddress() const { return initiatorLogicalAddress; }
  bool getReplyMode() const { return replyMode; }
  bool getIncrementMode() const { return incrementMode; }
  bool getVerifyMode() const { return verifyMode; }

  void setInitiatorLogicalAddress(u8 logicalAddress) { initiatorLogicalAddress = logicalAddress; }
  void setReplyMode(bool replyMode) { this->replyMode = replyMode; }
  void setIncrementMode(bool incrementMode) { this->incrementMode = incrementMode; }
  void setVerifyMode(bool verifyMode) { this->verifyMode = verifyMode; }

  static constexpr double DefaultTimeoutDuration = 1000.0;
  static const bool DefaultIncrementMode = true;
  static const bool DefaultVerifyMode = true;
  static const bool DefaultReplyMode = true;

 private:
  void setRMAPTransactionOptions(RMAPTransaction& transaction) {
    transaction.commandPacket->setIncrementFlag(incrementMode);
    transaction.commandPacket->setVerifyFlag(verifyMode);
    if (transaction.commandPacket->isRead() && transaction.commandPacket->isCommand()) {
      transaction.commandPacket->setReplyFlag(true);
    } else {
      transaction.commandPacket->setReplyFlag(replyMode);
    }
  }

 private:
  RMAPEnginePtr rmapEngine_;
  RMAPPacketPtr commandPacket;
  RMAPPacketPtr replyPacket;
  std::mutex transactionLock_;
  RMAPTransaction transaction{};

  u8 initiatorLogicalAddress = SpaceWireProtocol::DefaultLogicalAddress;
  bool incrementMode = DefaultIncrementMode;
  bool verifyMode = DefaultVerifyMode;
  bool replyMode = DefaultReplyMode;
};
#endif
