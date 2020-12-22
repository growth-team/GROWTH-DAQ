#include <algorithm>
#include <functional>
#include <iostream>
#include "spacewire/serialport.hh"

SerialPort::SerialPort(const std::string& deviceName, size_t baudRate) {
  serial::Timeout timeout(10, 10, 10, 10, 10);
  serial_.reset(new serial::Serial(deviceName, baudRate = 9600, timeout));
  serial_->open();
}

void SerialPort::close() {}

void SerialPort::send(const u8* buffer, size_t length) { serial_->write(buffer, length); }

size_t SerialPort::receive(u8* buffer, size_t length) { return serial_->read(buffer, length); }

inline size_t SerialPort::receiveLoopUntilSpecifiedLengthCompletes(u8* data, u32 length) {
  return receive(data, length, true);
}

size_t SerialPort::receive(u8* buffer, u32 length, bool waitUntilSpecifiedLengthCompletes) {
  std::lock_guard<std::mutex> lock{mutex_};

  size_t remainingLength = length;
  size_t nReceivedBytes = 0;
_SerialPort_receive_loop:  //
  nReceivedBytes += receive(buffer, remainingLength);

  if (nReceivedBytes > length) {
    using namespace std;
    cerr << "SerialPort::receive(): too long data (" << nReceivedBytes << " bytes) received against specified "
         << length << " bytes" << endl;
    throw std::runtime_error("Too large data received");
  }

  if (!waitUntilSpecifiedLengthCompletes) {
    return nReceivedBytes;
  } else {
    remainingLength = remainingLength - nReceivedBytes;
    if (remainingLength == 0) {
      return length;
    }
    goto _SerialPort_receive_loop;
  }
}
