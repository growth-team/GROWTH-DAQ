#include "spacewire/rmapengine.hh"

#include "spacewire/rmapinitiator.hh"
#include "spacewire/spacewireif.hh"
#include "spacewire/spacewireutil.hh"
#include "spacewire/types.hh"

#include "spdlog/spdlog.h"

RMAPEngine::RMAPEngine(SpaceWireIF* spwif, const ReceivedPacketOption& receivedPacketOption)
    : spwif(spwif), receivedPacketOption_(receivedPacketOption) {
  initialize();
}
RMAPEngine::~RMAPEngine() {
  stopped_ = true;
  spwif->cancelReceive();
  if (runThread.joinable()) {
    spdlog::info("Waiting for RMAPEngine receive thread to join");
    runThread.join();
    spdlog::info("RMAPEngine receive thread joined");
  }
}

void RMAPEngine::start() {
  if (!spwif) {
    throw RMAPEngineException(RMAPEngineException::SpaceWireInterfaceIsNotSet);
  }
  stopped_ = false;
  runThread = std::thread(&RMAPEngine::run, this);
}

void RMAPEngine::run() {
  hasStopped_ = false;

  spwif->setTimeoutDuration(DefaultReceiveTimeoutDurationInMicroSec);
  while (!stopped_) {
    try {
      auto rmapPacket = receivePacket();
      if (rmapPacket) {
        if (rmapPacket->isCommand()) {
          nDiscardedReceivedCommandPackets++;
        } else {
          rmapReplyPacketReceived(std::move(rmapPacket));
        }
      }
    } catch (const RMAPPacketException& e) {
      if (!stopped_) {
        spdlog::warn("RMAPPacketException in RMAPEngine::run() {}", e.what());
      }
    } catch (const RMAPEngineException& e) {
      if (!stopped_) {
        spdlog::warn("RMAPEngineException in RMAPEngine::run() {}", e.toString());
      }
    }
  }
  stopped_ = true;
  hasStopped_ = true;
}

void RMAPEngine::stop() {
  spdlog::info("Stopping RMAPEngine");
  if (!stopped_) {
    stopped_ = true;
    spwif->cancelReceive();

    while (!hasStopped_) {
      using namespace std::chrono_literals;
      std::this_thread::sleep_for(50ms);
    }
  }
  spdlog::info("RMAPEngine has stopped");
}

TransactionID RMAPEngine::initiateTransaction(RMAPPacket* commandPacket, RMAPInitiator* rmapInitiator) {
  if (!isStarted()) {
    throw RMAPEngineException(RMAPEngineException::RMAPEngineIsNotStarted);
  }
  const auto transactionID = getNextAvailableTransactionID();
  // register the transaction to management list if Reply is required
  if (commandPacket->isReplyFlagSet()) {
    std::lock_guard guard(transactionIDMutex);
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

bool RMAPEngine::isTransactionIDAvailable(u16 transactionID) {
  std::lock_guard guard(transactionIDMutex);
  return transactions.find(transactionID) == transactions.end();
}

void RMAPEngine::putBackRMAPPacketInstnce(RMAPPacketPtr packet) {
  std::lock_guard<std::mutex> guard(freeRMAPPacketListMutex_);
  freeRMAPPacketList_.emplace_back(std::move(packet));
}

void RMAPEngine::initialize() {
  std::lock_guard guard(transactionIDMutex);
  latestAssignedTransactionID = 0;
  availableTransactionIDList.clear();
  for (size_t i = 0; i < MaximumTIDNumber; i++) {
    availableTransactionIDList.push_back(i);
  }
}

u16 RMAPEngine::getNextAvailableTransactionID() {
  std::lock_guard guard(transactionIDMutex);
  if (!availableTransactionIDList.empty()) {
    const auto tid = availableTransactionIDList.front();
    availableTransactionIDList.pop_front();
    return tid;
  } else {
    throw RMAPEngineException(RMAPEngineException::TooManyConcurrentTransactions);
  }
}

void RMAPEngine::deleteTransactionIDFromDB(TransactionID transactionID) {
  std::lock_guard guard(transactionIDMutex);
  const auto& it = transactions.find(transactionID);
  if (it != transactions.end()) {
    transactions.erase(it);
    releaseTransactionID(transactionID);
  }
}

void RMAPEngine::releaseTransactionID(u16 transactionID) {
  std::lock_guard guard(transactionIDMutex);
  availableTransactionIDList.push_back(transactionID);
}

RMAPPacketPtr RMAPEngine::receivePacket() {
  try {
    spwif->receive(&receivePacketBuffer_);
  } catch (const SpaceWireIFException& e) {
  }

  RMAPPacketPtr packet = reuseOrCreateRMAPPacket();
  try {
    packet->setDataCRCIsChecked(!receivedPacketOption_.skipDataCrcCheck);
    packet->interpretAsAnRMAPPacket(&receivePacketBuffer_, receivedPacketOption_.skipConstructingWholePacketVector);
  } catch (const RMAPPacketException& e) {
    nDiscardedReceivedPackets++;
    return {};
  }
  return packet;
}

RMAPInitiator* RMAPEngine::resolveTransaction(const RMAPPacket* packet) {
  const auto transactionID = packet->getTransactionID();
  if (isTransactionIDAvailable(transactionID)) {  // if tid is not in use
    throw RMAPEngineException(RMAPEngineException::UnexpectedRMAPReplyPacketWasReceived);
  } else {  // if tid is registered to tid db
    // resolve transaction
    auto rmapInitiator = transactions[transactionID];
    // delete registered tid
    deleteTransactionIDFromDB(transactionID);
    // return resolved transaction
    return rmapInitiator;
  }
}

void RMAPEngine::rmapReplyPacketReceived(RMAPPacketPtr packet) {
  try {
    // find a corresponding command packet
    auto rmapInitiator = resolveTransaction(packet.get());
    // register reply packet to the resolved transaction
    rmapInitiator->replyReceived(std::move(packet));
  } catch (const RMAPEngineException& e) {
    // if not found, increment error counter
    nErrorneousReplyPackets++;
    putBackRMAPPacketInstnce(std::move(packet));
    return;
  }
}

RMAPPacketPtr RMAPEngine::reuseOrCreateRMAPPacket() {
  std::lock_guard<std::mutex> guard(freeRMAPPacketListMutex_);
  if (!freeRMAPPacketList_.empty()) {
    auto packet = std::move(freeRMAPPacketList_.back());
    freeRMAPPacketList_.pop_back();
    return packet;
  } else {
    return std::make_unique<RMAPPacket>();
  }
}
