#ifndef SPACEWIRE_SPACEWIREIFOVERUART_HH_
#define SPACEWIRE_SPACEWIREIFOVERUART_HH_

#include <chrono>
#include <thread>

#include "spacewire/spacewireif.hh"
#include "spacewire/types.hh"
#include "spacewiressdtpmoduleuart.hh"
#include "types.h"

/** SpaceWire IF class which transfers data over UART.
 */
class SpaceWireIFOverUART : public SpaceWireIF {
 public:
  static constexpr i32 BAUD_RATE = 230400;

  SpaceWireIFOverUART(const std::string& deviceName) : SpaceWireIF(), deviceName_(deviceName) {}
  ~SpaceWireIFOverUART() override = default;

  bool open() override {
    try {
      serial_ = std::make_unique<SerialPort>(deviceName_, BAUD_RATE);
      setTimeoutDuration(500000);
    } catch (...) {
      return false;
    }

    ssdtp_ = std::make_unique<SpaceWireSSDTPModuleUART>(serial_.get());
    isOpen_ = true;
    return isOpen_;
  }

  void close() override {
    ssdtp_->cancelReceive();
    serial_->close();
    isOpen_ = false;
  }

  void send(const u8* data, size_t length, EOPType eopType = EOPType::EOP) override {
    assert(!ssdtp_);
    try {
      ssdtp_->send(data, length, eopType);
    } catch (const SpaceWireSSDTPException& e) {
      throw SpaceWireIFException(e.what());
    }
  }

  void receive(std::vector<u8>* buffer) override {
    assert(!ssdtp_);
    try {
      EOPType eopType{};
      ssdtp_->receive(buffer, eopType);
      if (eopType == EOPType::EEP) {
        this->setReceivedPacketEOPMarkerType(EOPType::EEP);
        if (this->eepShouldBeReportedAsAnException_) {
          throw SpaceWireIFException("eep received");
        }
      } else {
        this->setReceivedPacketEOPMarkerType(EOPType::EOP);
      }
    } catch (SpaceWireSSDTPException& e) {
      throw SpaceWireIFException("timeout");
    } catch (SerialPortTimeoutException& e) {
      throw SpaceWireIFException("timeout");
    }
  }

  void setTimeoutDuration(f64 microsecond) override {}

  /** Cancels ongoing receive() method if any exist.
   */
  void cancelReceive() override {
    using namespace std::chrono_literals;
    static constexpr std::chrono::milliseconds WaitTimeAfterCancelReceive = 1500ms;
    ssdtp_->cancelReceive();
    std::this_thread::sleep_for(WaitTimeAfterCancelReceive);
  }

 private:
  const std::string deviceName_;
  std::unique_ptr<SpaceWireSSDTPModuleUART> ssdtp_;
  std::unique_ptr<SerialPort> serial_;
};

#endif
