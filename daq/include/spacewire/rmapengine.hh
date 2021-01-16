#ifndef SPACEWIRE_RMAPENGINE_HH_
#define SPACEWIRE_RMAPENGINE_HH_

#include <atomic>
#include <chrono>
#include <functional>
#include <list>
#include <map>
#include <mutex>
#include <queue>
#include <thread>

#include "spacewire/rmappacket.hh"
#include "spacewire/spacewireif.hh"
#include "spacewire/spacewireutil.hh"
#include "spacewire/types.hh"

class RMAPInitiator;

class RMAPEngineException : public Exception {
 public:
  enum {
    SpaceWireInterfaceIsNotSet,
    RMAPEngineIsNotStarted,
    TransactionIDIsNotAvailable,
    TooManyConcurrentTransactions,
    SpecifiedTransactionIDIsAlreadyInUse,
    PacketWasNotSentCorrectly,
    SpaceWireIFDisconnected,
    UnexpectedRMAPReplyPacketWasReceived
  };

  explicit RMAPEngineException(u32 status) : Exception(status) {}
  virtual ~RMAPEngineException() override = default;
  std::string toString() const override {
    std::string result;
    switch (status_) {
      case SpaceWireInterfaceIsNotSet:
        result = "SpaceWireInterfaceIsNotSet";
        break;
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
  struct ReceivedPacketOption {
    ReceivedPacketOption() {}
    ReceivedPacketOption(bool _skipDataCrcCheck, bool _skipConstructingWholePacketVector)
        : skipDataCrcCheck(_skipDataCrcCheck), skipConstructingWholePacketVector(_skipConstructingWholePacketVector) {}
    bool skipDataCrcCheck = false;
    bool skipConstructingWholePacketVector = false;
  };

  RMAPEngine(SpaceWireIF* spwif, const ReceivedPacketOption& receivedPacketOption = {});
  ~RMAPEngine();
  void start();
  void run();
  void stop();
  TransactionID initiateTransaction(RMAPPacket* commandPacket, RMAPInitiator* rmapInitiator);
  void cancelTransaction(TransactionID transactionID) { deleteTransactionIDFromDB(transactionID); }
  bool isStarted() const { return !stopped_; }
  bool hasStopped() const { return hasStopped_; }
  bool isTransactionIDAvailable(u16 transactionID);
  void putBackRMAPPacketInstnce(RMAPPacketPtr packet);
  size_t getNTransactions() const { return transactions.size(); }
  size_t getNAvailableTransactionIDs() const { return availableTransactionIDList.size(); }

 protected:
  void receivedCommandPacketDiscarded() { nErrorneousCommandPackets++; }
  void replyToReceivedCommandPacketCouldNotBeSent() { nTransactionsAbortedWhenReplying++; }

 private:
  void initialize();
  u16 getNextAvailableTransactionID();
  void deleteTransactionIDFromDB(TransactionID transactionID);

  void releaseTransactionID(u16 transactionID);
  RMAPPacketPtr receivePacket();

  RMAPInitiator* resolveTransaction(const RMAPPacket* packet);
  void rmapReplyPacketReceived(RMAPPacketPtr packet);
  RMAPPacketPtr reuseOrCreateRMAPPacket();

  static const size_t MaximumTIDNumber = 65536;
  static constexpr double DefaultReceiveTimeoutDurationInMicroSec = 200000;  // 200ms

  std::atomic<bool> stopped_{true};
  std::atomic<bool> hasStopped_{};

  size_t nDiscardedReceivedCommandPackets{};
  size_t nDiscardedReceivedPackets{};
  size_t nErrorneousReplyPackets{};
  size_t nErrorneousCommandPackets{};
  size_t nTransactionsAbortedWhenReplying{};
  size_t nErrorInRMAPReplyPacketProcessing{};

  std::map<u16, RMAPInitiator*> transactions;
  std::recursive_mutex transactionIDMutex;
  std::list<u16> availableTransactionIDList;
  u16 latestAssignedTransactionID;

  SpaceWireIF* spwif;
  std::thread runThread{};

  std::deque<RMAPPacketPtr> freeRMAPPacketList_;
  std::mutex freeRMAPPacketListMutex_;

  std::vector<u8> receivePacketBuffer_;

  const ReceivedPacketOption receivedPacketOption_;
};

#endif
