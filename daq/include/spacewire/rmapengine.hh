#ifndef SPACEWIRE_RMAPENGINE_HH_
#define SPACEWIRE_RMAPENGINE_HH_

#include <atomic>
#include <chrono>
#include <thread>

#include "spacewire/rmaptransaction.hh"
#include "spacewire/spacewireif.hh"
#include "spacewire/spacewireutilities.hh"

class RMAPEngineException : public CxxUtilities::Exception {
 public:
  enum {
    RMAPEngineIsNotStarted,
    TransactionIDIsNotAvailable,
    TooManyConcurrentTransactions,
    SpecifiedTransactionIDIsAlreadyInUse,
    PacketWasNotSentCorrectly,
    SpaceWireIFDisconnected,
    UnexpectedRMAPReplyPacketWasReceived
  };

 public:
  RMAPEngineException(u32 status) : CxxUtilities::Exception(status) {
    rmapPacketCausedThisException = NULL;
    causeIsRegistered = false;
  }

  virtual ~RMAPEngineException() override = default;

  RMAPEngineException(u32 status, RMAPPacket* packetCausedThisException) : CxxUtilities::Exception(status) {
    this->rmapPacketCausedThisException = packetCausedThisException;
    causeIsRegistered = true;
  }

  const RMAPPacket* getRMAPPacketCausedThisException() const { return rmapPacketCausedThisException; }

  bool isRMAPPacketCausedThisExceptionRegistered() const { return causeIsRegistered; }

  std::string toString() {
    std::string result;
    switch (status) {
      case RMAPEngineIsNotStarted:
        result = "RMAPEngineIsNotStarted";
        break;
      case TransactionIDIsNotAvailable:
        result = "TransactionIDIsNotAvailable";
        break;
      case TooManyConcurrentTransactions:
        result = "TooManyConcurrentTransactions";
        break;
      case SpecifiedTransactionIDIsAlreadyInUse:
        result = "SpecifiedTransactionIDIsAlreadyInUse";
        break;
      case PacketWasNotSentCorrectly:
        result = "PacketWasNotSentCorrectly";
        break;
      case SpaceWireIFDisconnected:
        result = "SpaceWireIFDisconnected";
        break;
      case UnexpectedRMAPReplyPacketWasReceived:
        result = "UnexpectedRMAPReplyPacketWasReceived";
        break;
      default:
        result = "Undefined status";
        break;
    }
    return result;
  }

 private:
  RMAPPacket* rmapPacketCausedThisException;
  bool causeIsRegistered;
};

class RMAPEngine : public CxxUtilities::Thread {
 public:
  RMAPEngine(SpaceWireIF* spwif) {
    initialize();
    this->spwif = spwif;
  }
  ~RMAPEngine() override = default;

  void run() {
    stopped = false;
    hasStopped = false;

    spwif->setTimeoutDuration(DefaultReceiveTimeoutDurationInMicroSec);
    while (!stopped) {
      try {
        RMAPPacket* rmapPacket = receivePacket();
        if (rmapPacket) {
          if (rmapPacket->isCommand()) {
            nDiscardedReceivedCommandPackets++;
          } else {
            rmapReplyPacketReceived(rmapPacket);
          }
        }
      } catch (RMAPPacketException& e) {
        std::cerr << "RMAPEngine::run() got RMAPPacketException " << e.toString() << std::endl;
        break;
      } catch (RMAPEngineException& e) {
        std::cerr << "RMAPEngine::run() got RMAPEngineException " << e.toString() << std::endl;
        break;
      }
    }
    stopped = true;
    hasStopped = true;
  }

  void stop() {
    if (!stopped) {
      stopped = true;
      spwif->cancelReceive();

      while (hasStopped != true) {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(50ms);
      }
    }
  }

  void initiateTransaction(RMAPTransaction* transaction) {
    if (!isStarted()) {
      throw RMAPEngineException(RMAPEngineException::RMAPEngineIsNotStarted);
    }
    transaction->state = RMAPTransaction::NotInitiated;
    RMAPPacket* commandPacket = transaction->getCommandPacket();
    const auto transactionID = getNextAvailableTransactionID();
    // register the transaction to management list
    // if Reply is required
    if (transaction->commandPacket->isReplyFlagSet()) {
      std::lock_guard<std::mutex> guard(transactionIDMutex);
      transactions[transactionID] = transaction;
    } else {
      // otherwise put back transaction Id to available id list
      returnTransactionID(transactionID);
    }
    // send a command packet
    commandPacket->setTransactionID(transactionID);
    commandPacket->constructPacket();
    std::lock_guard<std::mutex> stateGuard(transaction->stateMutex);
    spwif->send(commandPacket->getPacketBufferPointer());
    transaction->state = RMAPTransaction::Initiated;
  }

  void deleteTransactionIDFromDB(u16 transactionID) {
    // remove tid from management list
    std::lock_guard<std::mutex> guard(transactionIDMutex);
    const auto& it = transactions.find(transactionID);
    if (it != transactions.end()) {
      transactions.erase(it);
      // put back the transaction id to the available list
      returnTransactionID(transactionID);
    }
  }

  void cancelTransaction(RMAPTransaction* transaction) {
    RMAPPacket* commandPacket = transaction->getCommandPacket();
    const auto transactionID = commandPacket->getTransactionID();
    deleteTransactionIDFromDB(transactionID);
  }

  bool isStarted() const { return !stopped; }

  bool isTransactionIDAvailable(u16 transactionID) {
    std::lock_guard<std::mutex> guard(transactionIDMutex);
    return transactions.find(transactionID) == transactions.end();
  }

  size_t getNTransactions() const { return transactions.size(); }
  size_t getNAvailableTransactionIDs() const { return availableTransactionIDList.size(); }

 protected:
  void receivedCommandPacketDiscarded() { nErrorneousCommandPackets++; }
  void replyToReceivedCommandPacketCouldNotBeSent() { nTransactionsAbortedWhenReplying++; }

 private:
  void initialize() {
    std::lock_guard<std::mutex> guard(transactionIDMutex);
    latestAssignedTransactionID = 0;
    availableTransactionIDList.clear();
    for (size_t i = 0; i < MaximumTIDNumber; i++) {
      availableTransactionIDList.push_back(i);
    }
  }

  u16 getNextAvailableTransactionID() {
    std::lock_guard<std::mutex> guard(transactionIDMutex);
    if (!availableTransactionIDList.empty()) {
      const auto tid = availableTransactionIDList.front();
      availableTransactionIDList.pop_front();
      return tid;
    } else {
      throw RMAPEngineException(RMAPEngineException::TooManyConcurrentTransactions);
    }
  }

  void returnTransactionID(u16 transactionID) {
    std::lock_guard<std::mutex> guard(transactionIDMutex);
    availableTransactionIDList.push_back(transactionID);
  }

  RMAPPacket* receivePacket() {
    std::vector<u8>* buffer = new std::vector<u8>;
    try {
      spwif->receive(buffer);
    } catch (SpaceWireIFException& e) {
      delete buffer;
      if (e.status == SpaceWireIFException::Disconnected) {
        // tell run() that SpaceWireIF is disconnected
        throw RMAPEngineException(RMAPEngineException::SpaceWireIFDisconnected);
      } else {
        if (e.status == SpaceWireIFException::Timeout) {
          return NULL;
        } else {
          // tell run() that SpaceWireIF is disconnected
          throw RMAPEngineException(RMAPEngineException::SpaceWireIFDisconnected);
        }
      }
    }
    RMAPPacket* packet = new RMAPPacket();
    try {
      packet->interpretAsAnRMAPPacket(buffer);
    } catch (RMAPPacketException& e) {
      delete packet;
      delete buffer;
      nDiscardedReceivedPackets++;
      return NULL;
    }
    delete buffer;
    return packet;
  }

  RMAPTransaction* resolveTransaction(RMAPPacket* packet) {
    std::lock_guard<std::mutex> guard(transactionIDMutex);
    const auto transactionID = packet->getTransactionID();
    if (isTransactionIDAvailable(transactionID)) {  // if tid is not in use
      throw RMAPEngineException(RMAPEngineException::UnexpectedRMAPReplyPacketWasReceived, packet);
    } else {  // if tid is registered to tid db
      // resolve transaction
      RMAPTransaction* transaction = transactions[transactionID];
      // delete registered tid
      deleteTransactionIDFromDB(transactionID);
      // return resolved transaction
      return transaction;
    }
  }

  void rmapReplyPacketReceived(RMAPPacket* packet) {
    try {
      // find a corresponding command packet
      RMAPTransaction* transaction;
      try {
        transaction = this->resolveTransaction(packet);
      } catch (RMAPEngineException& e) {
        // if not found, increment error counter
        nErrorneousReplyPackets++;
        return;
      }
      // register reply packet to the resolved transaction
      transaction->replyPacket = packet;
      // update transaction state
      {
        std::lock_guard<std::mutex> stateGuard(transaction->stateMutex);
        transaction->setState(RMAPTransaction::ReplyReceived);
      }
      transaction->getCondition()->signal();
    } catch (CxxUtilities::MutexException& e) {
      std::cerr << "Fatal error in RMAPEngine::rmapReplyPacketReceived()... :-(" << std::endl;
      std::cerr << "RMAPEngine tries to recover normal operation, but may fail continuously." << std::endl;
      nErrorInRMAPReplyPacketProcessing++;
    } catch (...) {
      std::cerr << "Fatal error in RMAPEngine::rmapReplyPacketReceived()... :-(" << std::endl;
      std::cerr << "RMAPEngine tries to recover normal operation, but may fail continuously." << std::endl;
    }
  }

  static const size_t MaximumTIDNumber = 65536;
  static constexpr double DefaultReceiveTimeoutDurationInMicroSec = 200000;  // 200ms

  std::atomic<bool> stopped = true;
  std::atomic<bool> hasStopped{};

  size_t nDiscardedReceivedCommandPackets{};
  size_t nDiscardedReceivedPackets{};
  size_t nErrorneousReplyPackets{};
  size_t nErrorneousCommandPackets{};
  size_t nTransactionsAbortedWhenReplying{};
  size_t nErrorInRMAPReplyPacketProcessing{};

  std::map<u16, RMAPTransaction*> transactions;
  std::mutex transactionIDMutex;
  std::list<u16> availableTransactionIDList;
  u16 latestAssignedTransactionID;

  SpaceWireIF* spwif;
};

#endif
