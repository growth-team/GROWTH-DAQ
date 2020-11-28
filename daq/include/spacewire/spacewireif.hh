#ifndef SPACEWIRE_SPACEWIREIF_HH_
#define SPACEWIRE_SPACEWIREIF_HH_

#include "spacewire/types.hh"

class SpaceWireIF;

/** An abstract super class for SpaceWireIF-related actions.
 */
class SpaceWireIFAction {
 public:
  virtual ~SpaceWireIFAction() = default;
};

/** An abstract class which includes a method invoked when
 * Timecode is received.
 */
class SpaceWireIFActionTimecodeScynchronizedAction : public SpaceWireIFAction {
 public:
  /** Performs action.
   * @param[in] timecodeValue time code value
   */
  virtual void doAction(u8 timecodeValue) = 0;
};

/** An abstract class which includes a method invoked when
 * a SpaceWire interface is closed.
 */
class SpaceWireIFActionCloseAction : public SpaceWireIFAction {
 public:
  /** Performs action.
   * @param[in] spwif parent SpaceWireIF instance
   */
  virtual void doAction(SpaceWireIF* spwif) = 0;
};

/** An abstract class for encapsulation of a SpaceWire interface.
 *  This class provides virtual methods for opening/closing the interface
 *  and sending/receiving a packet via the interface.
 *  Physical or Virtual SpaceWire interface class should be created
 *  by inheriting this class.
 */
class SpaceWireIF {
  using TimecodeScynchronizedActionPtr = std::shared_ptr<SpaceWireIFActionTimecodeScynchronizedAction>;
  using CloseActionPtr = std::shared_ptr<SpaceWireIFActionCloseAction>;

 public:
  enum OpenCloseState { Closed, Opened };

 protected:
  enum OpenCloseState state;

 public:
  SpaceWireIF() { state = Closed; }

  virtual ~SpaceWireIF() {}

  enum OpenCloseState getState() { return state; }

  virtual void open() = 0;

  virtual void close() { invokespacewireIFCloseActions(); }

  virtual void send(u8* data, size_t length, EOPType eopType = EOPType::EOP) = 0;

  virtual void receive(u8* buffer, EOPType& eopType, size_t maxLength, size_t& length) {
    std::vector<u8>* packet = this->receive();
    size_t packetSize = packet->size();
    if (packetSize == 0) {
      length = 0;
    } else {
      length = packetSize;
      if (packetSize <= maxLength) {
        memcpy(buffer, &(packet->at(0)), packetSize);
      } else {
        memcpy(buffer, &(packet->at(0)), maxLength);
        delete packet;
        throw SpaceWireIFException(SpaceWireIFException::ReceiveBufferTooSmall);
      }
    }
    delete packet;
  }
  virtual std::vector<u8>* receive() {
    std::vector<u8>* buffer = new std::vector<u8>();
    try {
      this->receive(buffer);
      return buffer;
    } catch (SpaceWireIFException& e) {
      delete buffer;
      throw e;
    }
  }

  virtual void receive(std::vector<u8>* buffer) = 0;

  virtual void emitTimecode(u8 timeIn, u8 controlFlagIn = 0x00) = 0;

  virtual void setTxLinkRate(u32 linkRateMHz) = 0;

  virtual u32 getTxLinkRate() = 0;

  virtual void setTimeoutDuration(f64 microsecond) = 0;

  f64 getTimeoutDurationInMicroSec() { return timeoutDurationInMicroSec_; }

  /** A method sets (adds) an action against getting Timecode.
   */
  void addTimecodeAction(TimecodeScynchronizedActionPtr action) {
    for (size_t i = 0; i < timecodeSynchronizedActions_.size(); i++) {
      if (action == timecodeSynchronizedActions_[i]) {
        return;  // already registered
      }
    }
    timecodeSynchronizedActions_.push_back(action);
  }

  void clearTimecodeSynchronizedActions() { timecodeSynchronizedActions_.clear(); }

  void invokeTimecodeSynchronizedActions(u8 tickOutValue) {
    for (const auto& action : timecodeSynchronizedActions_) {
      action->doAction(tickOutValue);
    }
  }

  void addCloseAction(CloseActionPtr action) {
    const auto contains = std::find(closeActions_.begin(), closeActions_.end(), action);
    if (!contains) {
      closeActions_.push_back(action);
    }
  }

  void clearspacewireIFCloseActions_() { closeActions_.clear(); }

  /** Cancels ongoing receive() method if any exist.
   */
  virtual void cancelReceive() = 0;

 protected:
  void invokespacewireIFCloseActions() {
    for (const auto& action : closeActions_) {
      action->doAction(this);
    }
  }

  f64 timeoutDurationInMicroSec_{1e6};

 private:
  std::vector<TimecodeScynchronizedActionPtr> timecodeSynchronizedActions_;
  std::vector<CloseActionPtr> closeActions_;
};
#endif /* SPACEWIREIF_HH_ */
