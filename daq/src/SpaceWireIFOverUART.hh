/*
 ============================================================================
 SpaceWire/RMAP Library is provided under the MIT License.
 ============================================================================

 Copyright (c) 2006-2013 Takayuki Yuasa and The Open-source SpaceWire Project

 Permission is hereby granted, free of charge, to any person obtaining a
 copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:

 The above copyright notice and this permission notice shall be included
 in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
/*
 * SpaceWireIFOverTCP.hh
 *
 *  Created on: Dec 12, 2011
 *      Author: yuasa
 */

#ifndef SPACEWIREIFOVERUART_HH_
#define SPACEWIREIFOVERUART_HH_

#include "CxxUtilities/CxxUtilities.hh"

#include "SpaceWireRMAPLibrary/SpaceWireIF.hh"
#include "SpaceWireRMAPLibrary/SpaceWireSSDTPModule.hh"
#include "SpaceWireRMAPLibrary/SpaceWireUtilities.hh"
#include "SpaceWireSSDTPModuleUART.hh"

/** SpaceWire IF class which transfers data over UART.
 */
class SpaceWireIFOverUART : public SpaceWireIF, public SpaceWireIFActionTimecodeScynchronizedAction {
 public:
  static const int BAUD_RATE = 230400;

 public:
  static constexpr f64 WaitTimeAfterCancelReceive = 1500;  // ms

 private:
  std::string deviceName;
  SpaceWireSSDTPModuleUART* ssdtp;
  SerialPort* serialPort;

 public:
  /** Constructor.
   */
  SpaceWireIFOverUART(std::string deviceName) : SpaceWireIF(), deviceName(deviceName) {}

 public:
  virtual ~SpaceWireIFOverUART() {}

 public:
  void open() throw(SpaceWireIFException) {
    using namespace std;
    using namespace std;
    using namespace CxxUtilities;
    ssdtp = NULL;
    try {
      serialPort = new SerialPort(deviceName, BAUD_RATE);
      setTimeoutDuration(500000);
    } catch (SerialPortException& e) {
      throw SpaceWireIFException(SpaceWireIFException::OpeningConnectionFailed);
    } catch (...) { throw SpaceWireIFException(SpaceWireIFException::OpeningConnectionFailed); }

    ssdtp = new SpaceWireSSDTPModuleUART(serialPort);
    ssdtp->setTimeCodeAction(this);
    state = Opened;
  }

 public:
  void close() throw(SpaceWireIFException) {
    using namespace CxxUtilities;
    using namespace std;
    ssdtp->cancelReceive();
    serialPort->close();
  }

 public:
  void send(u8* data, size_t length,
            SpaceWireEOPMarker::EOPType eopType = SpaceWireEOPMarker::EOP) throw(SpaceWireIFException) {
    using namespace std;
    if (ssdtp == NULL) { throw SpaceWireIFException(SpaceWireIFException::LinkIsNotOpened); }
    try {
      ssdtp->send(data, length, eopType);
    } catch (SpaceWireSSDTPException& e) {
      if (e.getStatus() == SpaceWireSSDTPException::Timeout) {
        throw SpaceWireIFException(SpaceWireIFException::Timeout);
      } else {
        throw SpaceWireIFException(SpaceWireIFException::Disconnected);
      }
    }
  }

 public:
  void receive(std::vector<u8>* buffer) throw(SpaceWireIFException) {
    if (ssdtp == NULL) { throw SpaceWireIFException(SpaceWireIFException::LinkIsNotOpened); }
    try {
      u32 eopType;
      ssdtp->receive(buffer, eopType);
      if (eopType == SpaceWireEOPMarker::EEP) {
        this->setReceivedPacketEOPMarkerType(SpaceWireIF::EEP);
        if (this->eepShouldBeReportedAsAnException_) { throw SpaceWireIFException(SpaceWireIFException::EEP); }
      } else {
        this->setReceivedPacketEOPMarkerType(SpaceWireIF::EOP);
      }
    } catch (SpaceWireSSDTPException& e) {
      if (e.getStatus() == SpaceWireSSDTPException::Timeout) {
        throw SpaceWireIFException(SpaceWireIFException::Timeout);
      }
      using namespace std;
      throw SpaceWireIFException(SpaceWireIFException::Disconnected);
    } catch (SerialPortException& e) {
      if (e.getStatus() == SerialPortException::Timeout) { throw SpaceWireIFException(SpaceWireIFException::Timeout); }
      using namespace std;
      throw SpaceWireIFException(SpaceWireIFException::Disconnected);
    }
  }

 public:
  void emitTimecode(u8 timeIn, u8 controlFlagIn = 0x00) throw(SpaceWireIFException) {
    using namespace std;
    if (ssdtp == NULL) { throw SpaceWireIFException(SpaceWireIFException::LinkIsNotOpened); }
    timeIn = timeIn % 64 + (controlFlagIn << 6);
    try {
      // emit timecode via SSDTP module
      ssdtp->sendTimeCode(timeIn);
    } catch (SpaceWireSSDTPException& e) {
      if (e.getStatus() == SpaceWireSSDTPException::Timeout) {
        throw SpaceWireIFException(SpaceWireIFException::Timeout);
      }
      throw SpaceWireIFException(SpaceWireIFException::Disconnected);
    } catch (SerialPortException& e) {
      if (e.getStatus() == SerialPortException::Timeout) { throw SpaceWireIFException(SpaceWireIFException::Timeout); }
      throw SpaceWireIFException(SpaceWireIFException::Disconnected);
    }
    // invoke timecode synchronized action
    if (timecodeSynchronizedActions.size() != 0) { this->invokeTimecodeSynchronizedActions(timeIn); }
  }

 public:
  virtual void setTxLinkRate(u32 linkRateType) throw(SpaceWireIFException) {
    using namespace std;
    cerr << "SpaceWireIFOverIPClient::setTxLinkRate() is not implemented." << endl;
    cerr << "Please use SpaceWireIFOverIPClient::setTxDivCount() instead." << endl;
    throw SpaceWireIFException(SpaceWireIFException::FunctionNotImplemented);
  }

 public:
  virtual u32 getTxLinkRateType() throw(SpaceWireIFException) {
    using namespace std;
    cerr << "SpaceWireIFOverIPClient::getTxLinkRate() is not implemented." << endl;
    throw SpaceWireIFException(SpaceWireIFException::FunctionNotImplemented);
  }

 public:
  void setTxDivCount(u8 txdivcount) {}

 public:
  void setTimeoutDuration(f64 microsecond) throw(SpaceWireIFException) {
    serialPort->setTimeout(microsecond / 1000.);
    timeoutDurationInMicroSec = microsecond;
  }

 public:
  u8 getTimeCode() throw(SpaceWireIFException) {
    if (ssdtp == NULL) { throw SpaceWireIFException(SpaceWireIFException::LinkIsNotOpened); }
    return ssdtp->getTimeCode();
  }

 public:
  void doAction(u8 timecode) { this->invokeTimecodeSynchronizedActions(timecode); }

 public:
  SpaceWireSSDTPModuleUART* getSSDTPModule() { return ssdtp; }

 public:
  /** Cancels ongoing receive() method if any exist.
   */
  void cancelReceive() {
    ssdtp->cancelReceive();
    CxxUtilities::Condition c;
    c.wait(WaitTimeAfterCancelReceive);
  }
};

/** History
 * 2008-08-26 file created (Takayuki Yuasa)
 * 2011-10-21 rewritten (Takayuki Yuasa)
 */

#endif /* SPACEWIREIFOVERUART_HH_ */
