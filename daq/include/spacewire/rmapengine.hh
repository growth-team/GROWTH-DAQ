#ifndef SPACEWIRE_RMAPENGINE_HH_
#define SPACEWIRE_RMAPENGINE_HH_

#include <atomic>
#include <chrono>
#include <functional>
#include <queue>
#include <thread>

#include "spacewire/rmapinitiator.hh"
#include "spacewire/rmappacket.hh"
#include "spacewire/spacewireif.hh"
#include "spacewire/spacewireutilities.hh"
#include "spacewire/types.hh"

class RMAPEngineException : public Exception {
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

  RMAPEngineException(u32 status) : Exception(status) {}
  virtual ~RMAPEngineException() override = default;
  std::string toString() const override {
    std::string result;
    switch (status_) {
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
};

class RMAPEngine {
 public:
  RMAPEngine(SpaceWireIF* spwif) : spwif(spwif) { initialize(); }
  ~RMAPEngine() {
    stopped = true;
    if (runThread.joinable()) {
      runThread.join();
    }
  }

  void start() { runThread = std::thread(&RMAPEngine::run, this); }

  void run() {
    stopped = false;
    hasStopped = false;

    spwif->setTimeoutDuration(DefaultReceiveTimeoutDurationInMicroSec);
    while (!stopped) {
      try {
        std::unique_ptr<RMAPPacket> rmapPacket = receivePacket();
        if (rmapPacket) {
          if (rmapPacket->isCommand()) {
            nDiscardedReceivedCommandPackets++;
          } else {
            rmapReplyPacketReceived(std::move(rmapPacket));
          }
        }
      } catch (const RMAPPacketException& e) {
        break;
      } catch (const RMAPEngineException& e) {
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

  TransactionID initiateTransaction(RMAPPacket* commandPacket, RMAPInitiator* rmapInitiator) {
    if (!isStarted()) {
      throw RMAPEngineException(RMAPEngineException::RMAPEngineIsNotStarted);
    }
    const auto transactionID = getNextAvailableTransactionID();
    // register the transaction to management list if Reply is required
    if (commandPacket->isReplyFlagSet()) {
      std::lock_guard<std::mutex> guard(transactionIDMutex);
      transactions[transactionID] = rmapInitiator;
    } else {
      // otherwise put back transaction Id to available id list
      releaseTransactionID(transactionID);
    }
    // send a command packet
    commandPacket->setTransactionID(transactionID);
    commandPacket->constructPacket();
    const auto packet = commandPacket->getPacketBufferPointer();
    spwif->send(packet->data(), packet->size());
    return transactionID;
  }

  void cancelTransaction(TransactionID transactionID) { deleteTransactionIDFromDB(transactionID); }

  bool isStarted() const { return !stopped; }

  bool isTransactionIDAvailable(u16 transactionID) {
    std::lock_guard<std::mutex> guard(transactionIDMutex);
    return transactions.find(transactionID) == transactions.end();
  }

  void putBackRMAPPacketInstnce(RMAPPacketPtr&& packet) {
    std::lock_guard<std::mutex> guard(freeRMAPPacketListMutex_);
    freeRMAPPacketList_.push_back(packet);
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

  void deleteTransactionIDFromDB(TransactionID transactionID) {
    std::lock_guard<std::mutex> guard(transactionIDMutex);
    const auto& it = transactions.find(transactionID);
    if (it != transactions.end()) {
      transactions.erase(it);
      releaseTransactionID(transactionID);
    }
  }

  void releaseTransactionID(u16 transactionID) {
    std::lock_guard<std::mutex> guard(transactionIDMutex);
    availableTransactionIDList.push_back(transactionID);
  }
  std::unique_ptr<RMAPPacket> receivePacket() {
    try {
      spwif->receive(&receivePacketBuffer_);
    } catch (const SpaceWireIFException& e) {
      if (e.getStatus() == SpaceWireIFException::Disconnected) {
        // tell run() that SpaceWireIF is disconnected
        throw RMAPEngineException(RMAPEngineException::SpaceWireIFDisconnected);
      } else {
        if (e.getStatus() == SpaceWireIFException::Timeout) {
          return nullptr;
        } else {
          // tell run() that SpaceWireIF is disconnected
          throw RMAPEngineException(RMAPEngineException::SpaceWireIFDisconnected);
        }
      }
    }
    std::unique_ptr<RMAPPacket> packet = reuseOrCreateRMAPPacket();
    try {
      packet->interpretAsAnRMAPPacket(&receivePacketBuffer_);
    } catch (const RMAPPacketException& e) {
      nDiscardedReceivedPackets++;
      return nullptr;
    }
    return packet;
  }

  RMAPInitiator* resolveTransaction(const RMAPPacket* packet) {
    std::lock_guard<std::mutex> guard(transactionIDMutex);
    const auto transactionID = packet->getTransactionID();
    if (isTransactionIDAvailable(transactionID)) {  // if tid is not in use
      throw RMAPEngineException(RMAPEngineException::UnexpectedRMAPReplyPacketWasReceived);
    } else {  // if tid is registered to tid db
      // resolve transaction
      RMAPInitiator* rmapInitiator = transactions[transactionID];
      // delete registered tid
      deleteTransactionIDFromDB(transactionID);
      // return resolved transaction
      return rmapInitiator;
    }
  }

  void rmapReplyPacketReceived(std::unique_ptr<RMAPPacket> packet) {
    try {
      // find a corresponding command packet
      auto rmapInitiator = resolveTransaction(packet.get());
      // register reply packet to the resolved transaction
      rmapInitiator->replyReceived(std::move(packet));
    } catch (const RMAPEngineException& e) {
      // if not found, increment error counter
      nErrorneousReplyPackets++;
      return;
    }
  }

  RMAPPacketPtr reuseOrCreateRMAPPacket() {
    std::lock_guard<std::mutex> guard(freeRMAPPacketListMutex_);
    if (!freeRMAPPacketList_.empty()) {
      RMAPPacketPtr packet{freeRMAPPacketList_.back()};
      freeRMAPPacketList_.pop_back();
      return packet;
    } else {
      return RMAPPacketPtr(new RMAPPacket);
    }
  }

  static const size_t MaximumTIDNumber = 65536;
  static constexpr double DefaultReceiveTimeoutDurationInMicroSec = 200000;  // 200ms

  std::atomic<bool> stopped{true};
  std::atomic<bool> hasStopped{};

  size_t nDiscardedReceivedCommandPackets{};
  size_t nDiscardedReceivedPackets{};
  size_t nErrorneousReplyPackets{};
  size_t nErrorneousCommandPackets{};
  size_t nTransactionsAbortedWhenReplying{};
  size_t nErrorInRMAPReplyPacketProcessing{};

  std::map<u16, RMAPInitiator*> transactions;
  std::mutex transactionIDMutex;
  std::list<u16> availableTransactionIDList;
  u16 latestAssignedTransactionID;

  SpaceWireIF* spwif;
  std::thread runThread{};

  std::deque<RMAPPacketPtr> freeRMAPPacketList_;
  std::mutex freeRMAPPacketListMutex_;

  std::vector<u8> receivePacketBuffer_;
};

#endif
