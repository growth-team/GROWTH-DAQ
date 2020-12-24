#include "spacewire/serialport.hh"

#include "spdlog/spdlog.h"

#include <algorithm>
#include <functional>
#include <iostream>

size_t SerialPort::receiveLoopUntilSpecifiedLengthCompletes(u8* data, u32 length) {
  return receive(data, length, true);
}

size_t SerialPort::receive(u8* buffer, u32 length, bool waitUntilSpecifiedLengthCompletes) {
  size_t remainingLength = length;
  size_t nReceivedBytes = 0;
_SerialPort_receive_loop:  //
  nReceivedBytes += receive(buffer, remainingLength);

  if (nReceivedBytes > length) {
    const std::string msg = fmt::format(
        "SerialPort::receive(): too long data ({} bytes) received against specified {} bytes", nReceivedBytes, length);
    spdlog::error(msg);
    throw SerialPortException(SerialPortException::TooLargeDataReceived);
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

//------------------------------------------------------------------------------------------
SerialPortLibSerial::SerialPortLibSerial(const std::string& deviceName, size_t baudRate) {
  serial::Timeout timeout(10, 10, 10, 10, 10);
  serial_.reset(new serial::Serial(deviceName, baudRate = 9600, timeout));
  serial_->open();
}

void SerialPortLibSerial::close() {}

void SerialPortLibSerial::send(const u8* buffer, size_t length) { serial_->write(buffer, length); }

size_t SerialPortLibSerial::receive(u8* buffer, size_t length) { return serial_->read(buffer, length); }

//------------------------------------------------------------------------------------------
SerialPortBoostAsio::SerialPortBoostAsio(const std::string& deviceName, size_t baudRate) {
  port.reset(new asio::serial_port(io, deviceName.c_str()));
  port->set_option(asio::serial_port_base::baud_rate(baudRate));
  port->set_option(asio::serial_port_base::character_size(8));
  port->set_option(asio::serial_port_base::flow_control(asio::serial_port_base::flow_control::none));
  port->set_option(asio::serial_port_base::parity(asio::serial_port_base::parity::none));
  port->set_option(asio::serial_port_base::stop_bits(asio::serial_port_base::stop_bits::one));

  // for non-blocking
  // boost::thread thr_io(boost::bind(&asio::io_service::run, &io));
}

void SerialPortBoostAsio::close() { port->close(); }

void SerialPortBoostAsio::send(const u8* sendBuffer, size_t length) {
  asio::write(*port, asio::buffer(sendBuffer, length));
}

size_t SerialPortBoostAsio::receive(u8* data, size_t length) {
  i32 result = 0;
  size_t remainingLength = length;
  size_t readDoneLength = 0;
  size_t nReceivedBytes = 0;

  try {
  _SerialPort_receive_loop:  //
    auto buffer = asio::buffer((uint8_t*)data + readDoneLength, remainingLength);
    receiveWithTimeout(buffer, nReceivedBytes, std::chrono::milliseconds(1000));

    if (nReceivedBytes > length) {
      using namespace std;
      cerr << "SerialPort::receive(): too long data (" << nReceivedBytes << " bytes) received against specified "
           << length << " bytes" << endl;
      throw SerialPortException(SerialPortException::TooLargeDataReceived);
    }

    return nReceivedBytes;
  } catch (asio::system_error& e) {
    throw SerialPortException(SerialPortException::Timeout);
  }
}
