#include <algorithm>
#include <boost/thread.hpp>
#include <functional>
#include <iostream>

#include "SerialPort.hh"

  SerialPortException::SerialPortException(int status) : CxxUtilities::Exception(status) {}

  std::string SerialPortException::toString() {
    switch (this->getStatus()) {
      case Timeout:
        return "Timeout";
      case SerialPortClosed:
        return "SerialPortClosed";
      case TooLargeDataReceived:
        return "TooLargeDataReceived";
      default:
        return "UndefinedException";
    }
  }


  void SerialPort::read_callback(const boost::system::error_code &e, std::size_t size, boost::array<char, ReadBufferSize> r) {
    std::cout << "read :" << size << "byte[s] " << std::endl;
    std::cout.write(r.data(), size);
  }

  void SerialPort::write_callback(const boost::system::error_code &e, std::size_t size) {
    std::cout << "write :" << size << "byte[s]" << std::endl;
  }

  SerialPort::SerialPort(std::string deviceName, size_t baudRate = 9600) : timeoutDurationObject(1000) {
    using namespace std;

    port.reset(new boost::asio::serial_port(io, deviceName.c_str()));
    port->set_option(boost::asio::serial_port_base::baud_rate(baudRate));
    port->set_option(boost::asio::serial_port_base::character_size(8));
    port->set_option(boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none));
    port->set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none));
    port->set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));

    // for non-blocking
    // boost::thread thr_io(boost::bind(&boost::asio::io_service::run, &io));

    setTimeout(1000);
  }

  void SerialPort::close() {
    port->close();
  }


  void SerialPort::send(std::vector<uint8_t> &sendBuffer) {
#ifdef DEBUG_SERIALPORT
    using namespace std;
    cout << "Send: ";
    SpaceWireUtilities::dumpPacket(sendBuffer);
#endif
    port->write_some(boost::asio::buffer(sendBuffer));
  }

  void SerialPort::send(uint8_t *sendBuffer, size_t length) {
    internalBufferForSend.resize(length);
    memcpy(&(internalBufferForSend[0]), sendBuffer, length);
#ifdef DEBUG_SERIALPORT
    using namespace std;
    cout << "Send: ";
    SpaceWireUtilities::dumpPacket(internalBufferForSend);
#endif
    port->write_some(boost::asio::buffer(internalBufferForSend));
  }

  template <typename MutableBufferSequence>
  void SerialPort::receiveWithTimeout(const MutableBufferSequence &buffers, size_t &nReceivedBytes,
                          const boost::asio::deadline_timer::duration_type &expiry_time) {
    mutex.lock();
    boost::optional<boost::system::error_code> timer_result;
    boost::asio::deadline_timer timer(port->get_io_service());
    timer.expires_from_now(expiry_time);
    timer.async_wait([&timer_result](const boost::system::error_code &error) { timer_result.reset(error); });

    boost::optional<boost::system::error_code> read_result;
    port->async_read_some(
        buffers, [&read_result, &nReceivedBytes](const boost::system::error_code &error, size_t _nReceivedBytes) {
          read_result.reset(error);
          nReceivedBytes = _nReceivedBytes;
        });

    port->get_io_service().reset();
    while (port->get_io_service().run_one()) {
      if (read_result)
        timer.cancel();
      else if (timer_result)
        port->cancel();
    }

    if (*read_result) {
      mutex.unlock();
      throw boost::system::system_error(*read_result);
    }

    mutex.unlock();
  }

  inline size_t SerialPort::receiveLoopUntilSpecifiedLengthCompletes(uint8_t *data, uint32_t length) throw(SerialPortException) {
    return receive(data, length, true);
  }

  size_t SerialPort::receive(uint8_t *data, uint32_t length,
                 bool waitUntilSpecifiedLengthCompletes = false) throw(SerialPortException) {
    int result          = 0;
    int remainingLength = length;
    int readDoneLength  = 0;
    size_t nReceivedBytes{};

    try {
    _SerialPort_receive_loop:  //
      receiveWithTimeout(boost::asio::buffer((uint8_t *)data + readDoneLength, remainingLength), nReceivedBytes,
                         timeoutDurationObject);

      if (nReceivedBytes > length) {
        using namespace std;
        cerr << "SerialPort::receive(): too long data (" << nReceivedBytes << " bytes) received against specified "
             << length << " bytes" << endl;
        throw SerialPortException(SerialPortException::TooLargeDataReceived);
      }

      if (waitUntilSpecifiedLengthCompletes == false) {
        return nReceivedBytes;
      } else {
        remainingLength = remainingLength - nReceivedBytes;
        readDoneLength  = readDoneLength + nReceivedBytes;
        if (remainingLength == 0) { return length; }
        goto _SerialPort_receive_loop;
      }
    } catch (boost::system::system_error &e) {
#ifdef DEBUG_SERIALPORT
      using namespace std;
      cout << "Exception in SerialPort::receive(): " << e.what() << endl;
#endif
      throw SerialPortException(SerialPortException::Timeout);
    }
  }

  void SerialPort::setTimeout(double timeoutDurationInMilliSec) {
    this->timeoutDurationInMilliSec = timeoutDurationInMilliSec;
    timeoutDurationObject           = boost::posix_time::millisec(static_cast<uint32_t>(timeoutDurationInMilliSec));
  }
