#ifndef SPACEWIRE_SPACEWIREIF_HH_
#define SPACEWIRE_SPACEWIREIF_HH_

#include "spacewire/types.hh"

class SpaceWireIF;

class SpaceWireIFException : public std::runtime_error {
 public:
  SpaceWireIFException(const std::string& message) : std::runtime_error(message) {}
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
        throw SpaceWireIFException("receive buffer too small");
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
  virtual void cancelReceive() = 0;
  virtual void setTimeoutDuration(f64 microsecond) = 0;

  f64 getTimeoutDurationInMicroSec() const { return timeoutDurationInMicroSec_; }

 protected:
  bool isOpen_ = false;
  f64 timeoutDurationInMicroSec_{1e6};
};
#endif /* SPACEWIREIF_HH_ */
