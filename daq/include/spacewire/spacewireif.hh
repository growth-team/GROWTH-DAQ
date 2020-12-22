#ifndef SPACEWIRE_SPACEWIREIF_HH_
#define SPACEWIRE_SPACEWIREIF_HH_

#include <cstring>

#include "spacewire/types.hh"

class SpaceWireIF;

class SpaceWireIFException : public Exception {
 public:
  enum {
    OpeningConnectionFailed,
    Disconnected,
    ReceiveFailed,
    SendFailed,
    Timeout,
    EEP,
    ReceiveBufferTooSmall,
    FunctionNotImplemented,
    LinkIsNotOpened
  };
  SpaceWireIFException(u32 status) : Exception(status) {}
  std::string toString() const override {
    std::string result;
    switch (status_) {
      case OpeningConnectionFailed:
        result = "OpeningConnectionFailed";
        break;
      case Disconnected:
        result = "Disconnected";
        break;
      case ReceiveFailed:
        result = ReceiveFailed;
        break;
      case SendFailed:
        result = "SendFailed";
        break;
      case Timeout:
        result = "Timeout";
        break;
      case EEP:
        result = "EEP";
        break;
      case ReceiveBufferTooSmall:
        result = "ReceiveBufferTooSmall";
        break;
      case FunctionNotImplemented:
        result = "FunctionNotImplemented";
        break;
      case LinkIsNotOpened:
        result = "LinkIsNotOpened";
        break;
      default:
        result = "Undefined status";
        break;
    }
    return result;
  }
};

/** An abstract class for encapsulation of a SpaceWire interface.
 *  This class provides virtual methods for opening/closing the interface
 *  and sending/receiving a packet via the interface.
 *  Physical or Virtual SpaceWire interface class should be created
 *  by inheriting this class.
 */
class SpaceWireIF {
 public:
  SpaceWireIF() = default;
  virtual ~SpaceWireIF() = default;

  virtual bool open() = 0;
  bool isOpen() const { return isOpen_; }

  virtual void close() = 0;

  virtual void send(const u8* data, size_t length, EOPType eopType = EOPType::EOP) = 0;
  virtual void receive(u8* buffer, EOPType& eopType, size_t maxLength, size_t& length) {
    std::vector<u8>* packet = this->receive();
    const size_t packetSize = packet->size();
    if (packetSize == 0) {
      length = 0;
    } else {
      length = packetSize;
      if (packetSize <= maxLength) {
        memcpy(buffer, packet->data(), packetSize);
      } else {
        memcpy(buffer, packet->data(), maxLength);
        delete packet;
        throw SpaceWireIFException(SpaceWireIFException::ReceiveBufferTooSmall);
      }
    }
    delete packet;
  }
  virtual std::vector<u8>* receive() {
    // TODO: replace with buffer pool
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
  virtual void cancelReceive() = 0;
  virtual void setTimeoutDuration(f64 microsecond) = 0;

  f64 getTimeoutDurationInMicroSec() const { return timeoutDurationInMicroSec_; }

 protected:
  bool isOpen_ = false;
  f64 timeoutDurationInMicroSec_{1e6};
};
#endif /* SPACEWIREIF_HH_ */
