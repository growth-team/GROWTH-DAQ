#include <algorithm>
#include <boost/thread.hpp>
#include <functional>
#include <iostream>

#include "SerialPort.hh"

SerialPortException::SerialPortException(int status) : CxxUtilities::Exception(status) {}

std::string SerialPortException::toString() const {
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

void SerialPort::read_callback(const boost::system::error_code& e, std::size_t size,
                               boost::array<char, ReadBufferSize> r) {
  std::cout << "read :" << size << "byte[s] " << std::endl;
  std::cout.write(r.data(), size);
}

void SerialPort::write_callback(const boost::system::error_code& e, std::size_t size) {
  std::cout << "write :" << size << "byte[s]" << std::endl;
}

SerialPort::SerialPort(std::string deviceName, size_t baudRate) : timeoutDurationObject_(1000) {
  using namespace std;

  port_.reset(new boost::asio::serial_port(io_, deviceName.c_str()));
  port_->set_option(boost::asio::serial_port_base::baud_rate(baudRate));
  port_->set_option(boost::asio::serial_port_base::character_size(8));
  port_->set_option(boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none));
  port_->set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none));
  port_->set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));

  // for non-blocking
  // boost::thread thr_io(boost::bind(&boost::asio::io_service::run, &io));

  setTimeout(1000);
}

void SerialPort::close() { port_->close(); }

void SerialPort::send(const std::vector<u8>& sendBuffer) {
#ifdef DEBUG_SERIALPORT
  using namespace std;
  cout << "Send: ";
  SpaceWireUtilities::dumpPacket(sendBuffer);
#endif
  port_->write_some(boost::asio::buffer(sendBuffer));
}

void SerialPort::send(const u8* sendBuffer, size_t length) {
  internalSendBuffer_.resize(length);
  memcpy(&(internalSendBuffer_[0]), sendBuffer, length);
#ifdef DEBUG_SERIALPORT
  using namespace std;
  cout << "Send: ";
  SpaceWireUtilities::dumpPacket(internalSendBuffer_);
#endif
  port_->write_some(boost::asio::buffer(internalSendBuffer_));
}

template <typename MutableBufferSequence>
void SerialPort::receiveWithTimeout(MutableBufferSequence& buffers, size_t& nReceivedBytes,
                                    const boost::asio::deadline_timer::duration_type& expiry_time) {
  std::lock_guard<std::mutex> guard(mutex_);
  boost::optional<boost::system::error_code> timer_result;
  boost::asio::deadline_timer timer(port_->get_io_service());
  timer.expires_from_now(expiry_time);
  timer.async_wait([&timer_result](const boost::system::error_code& error) { timer_result.reset(error); });

  boost::optional<boost::system::error_code> read_result;
  port_->async_read_some(
      buffers, [&read_result, &nReceivedBytes](const boost::system::error_code& error, size_t _nReceivedBytes) {
        read_result.reset(error);
        nReceivedBytes = _nReceivedBytes;
      });

  port_->get_io_service().reset();
  while (port_->get_io_service().run_one()) {
    if (read_result)
      timer.cancel();
    else if (timer_result)
      port_->cancel();
  }

  if (*read_result) {
    throw boost::system::system_error(*read_result);
  }
}

inline size_t SerialPort::receiveLoopUntilSpecifiedLengthCompletes(u8* data, u32 length) {
  return receive(data, length, true);
}

size_t SerialPort::receive(u8* data, u32 length, bool waitUntilSpecifiedLengthCompletes) {
  size_t remainingLength = length;
  size_t readDoneLength = 0;
  size_t nReceivedBytes{};

  try {
  _SerialPort_receive_loop:  //
    auto buffer = boost::asio::buffer(data + readDoneLength, remainingLength);
    receiveWithTimeout(buffer, nReceivedBytes, timeoutDurationObject_);

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
      readDoneLength = readDoneLength + nReceivedBytes;
      if (remainingLength == 0) {
        return length;
      }
      goto _SerialPort_receive_loop;
    }
  } catch (boost::system::system_error& e) {
#ifdef DEBUG_SERIALPORT
    using namespace std;
    cout << "Exception in SerialPort::receive(): " << e.what() << endl;
#endif
    throw SerialPortException(SerialPortException::Timeout);
  }
}

void SerialPort::setTimeout(f64 timeoutDurationInMilliSec) {
  this->timeoutDurationInMilliSec_ = timeoutDurationInMilliSec;
  timeoutDurationObject_ = boost::posix_time::millisec(static_cast<u32>(timeoutDurationInMilliSec));
}
