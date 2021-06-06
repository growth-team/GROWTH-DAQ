#ifndef SPACEWIRE_SPACEWIREIFOVERUART_HH_
#define SPACEWIRE_SPACEWIREIFOVERUART_HH_

#include "types.h"
#include "spacewire/spacewireif.hh"
#include "spacewire/types.hh"
#include "spacewiressdtpmoduleuart.hh"
#include "stringutil.hh"

#include <chrono>
#include <thread>
#include <cassert>

#include "spdlog/spdlog.h"

/** SpaceWire IF class which transfers data over UART.
 */
class SpaceWireIFOverUART : public SpaceWireIF {
 public:
  static constexpr i32 BAUD_RATE = 230400;

  SpaceWireIFOverUART(const std::string& deviceName) : SpaceWireIF(), deviceName_(deviceName) {}
  ~SpaceWireIFOverUART() override = default;

  bool open() override {

    serial_ = std::make_unique<SerialPortBoostAsio>(deviceName_, BAUD_RATE);
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
      if (e.getStatus() == SpaceWireSSDTPException::Timeout) {
        throw SpaceWireIFException(SpaceWireIFException::Timeout);
      } else {
        throw SpaceWireIFException(SpaceWireIFException::SendFailed);
      }
    }
  }

  void receive(std::vector<u8>* buffer) override {
    assert(!ssdtp_);
    try {
      EOPType eopType{};
      ssdtp_->receive(buffer, eopType);
      if (eopType == EOPType::EEP) {
        throw SpaceWireIFException(SpaceWireIFException::EEP);
      }
    } catch (const SpaceWireSSDTPException& e) {
      spdlog::debug("SpaceWireIFOverUART::receive() SpaceWireSSDTPException::{} ({})", e.toString(), e.getStatus());
      if (e.getStatus() == SpaceWireSSDTPException::Timeout) {
        throw SpaceWireIFException(SpaceWireIFException::Timeout);
      } else {
        throw SpaceWireIFException(SpaceWireIFException::ReceiveFailed);
      }
    } catch (const SerialPortException& e) {
      spdlog::debug("SpaceWireIFOverUART::receive() SerialPortException::{} ({})", e.toString(), e.getStatus());
      throw SpaceWireIFException(SpaceWireIFException::Timeout);
    }
  }

  void setTimeoutDuration(f64 microsecond) override {}

  /** Cancels ongoing receive() method if any exist.
   */
  void cancelReceive() override {
    ssdtp_->cancelReceive();
  }

 private:
  const std::string deviceName_;
  std::unique_ptr<SpaceWireSSDTPModuleUART> ssdtp_;
  std::unique_ptr<SerialPort> serial_;
};

#endif
