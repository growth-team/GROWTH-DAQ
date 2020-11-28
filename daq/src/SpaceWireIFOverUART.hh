#ifndef SPACEWIREIFOVERUART_HH_
#define SPACEWIREIFOVERUART_HH_

#include <chrono>
#include <thread>

#include "spacewire/spacewireif.hh"
#include "SpaceWireSSDTPModuleUART.hh"
#include "types.h"

/** SpaceWire IF class which transfers data over UART.
 */
class SpaceWireIFOverUART : public SpaceWireIF {
 public:
  static constexpr i32 BAUD_RATE = 230400;

  SpaceWireIFOverUART(const std::string& deviceName) : SpaceWireIF(), deviceName_(deviceName) {}

  ~SpaceWireIFOverUART() override = default;

  void open() override {
    try {
      serial_ = std::make_unique<SerialPort>(deviceName_, BAUD_RATE);
      setTimeoutDuration(500000);
    } catch (...) {
      throw SpaceWireIFException(SpaceWireIFException::OpeningConnectionFailed);
    }

    ssdtp_ = std::make_unique<SpaceWireSSDTPModuleUART>(serial_.get());
    state = Opened;
  }

  void close() override {
    ssdtp_->cancelReceive();
    serial_->close();
    state = Closed;
  }

  void send(u8* data, size_t length,
            SpaceWireEOPMarker::EOPType eopType = SpaceWireEOPMarker::EOP) override {
    using namespace std;
    assert(!ssdtp_);
    try {
      ssdtp_->send(data, length, eopType);
    } catch (SpaceWireSSDTPException& e) {
      if (e.getStatus() == SpaceWireSSDTPException::Timeout) {
        throw SpaceWireIFException(SpaceWireIFException::Timeout);
      } else {
        throw SpaceWireIFException(SpaceWireIFException::Disconnected);
      }
    }
  }

  void receive(std::vector<u8>* buffer) override {
    assert(!ssdtp_);
    try {
      u32 eopType{};
      ssdtp_->receive(buffer, eopType);
      if (eopType == SpaceWireEOPMarker::EEP) {
        this->setReceivedPacketEOPMarkerType(SpaceWireIF::EEP);
        if (this->eepShouldBeReportedAsAnException_) {
          throw SpaceWireIFException(SpaceWireIFException::EEP);
        }
      } else {
        this->setReceivedPacketEOPMarkerType(SpaceWireIF::EOP);
      }
    } catch (SpaceWireSSDTPException& e) {
      if (e.getStatus() == SpaceWireSSDTPException::Timeout) {
        throw SpaceWireIFException(SpaceWireIFException::Timeout);
      }
      using namespace std;
      throw SpaceWireIFException(SpaceWireIFException::Disconnected);
    } catch (SerialPortTimeoutException& e) {
      throw SpaceWireIFException(SpaceWireIFException::Timeout);
    }
  }

  void setTimeoutDuration(f64 microsecond) override {
    serial_->setTimeout(microsecond / 1000.);
    timeoutDurationInMicroSec = microsecond;
  }

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

#endif /* SPACEWIREIFOVERUART_HH_ */
