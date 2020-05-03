/*
 * SerialPort.hh
 *
 *  Created on: Jun 9, 2015
 *      Author: yuasa
 */

#ifndef SERIALPORT_HH_
#define SERIALPORT_HH_

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/optional.hpp>
#include <boost/system/system_error.hpp>
//#include <boost/bind1st.hpp>
#include <memory>
#include <vector>

#include "SpaceWireRMAPLibrary/SpaceWireUtilities.hh"

#include "CxxUtilities/CxxUtilities.hh"

class SerialPortException : public CxxUtilities::Exception {
 public:
  enum { Timeout, SerialPortClosed, TooLargeDataReceived };
  SerialPortException(int status);
  std::string toString() const;
};

struct SerialPort {
 public:
  static constexpr size_t ReadBufferSize = 10 * 1024;

 public:
  SerialPort(std::string deviceName, size_t baudRate = 9600);
  void read_callback(const boost::system::error_code& e, std::size_t size, boost::array<char, ReadBufferSize> r);
  void write_callback(const boost::system::error_code& e, std::size_t size);
  void close();
  void send(const std::vector<uint8_t>& sendBuffer);
  void send(const uint8_t* sendBuffer, size_t length);
  template <typename MutableBufferSequence>
  void receiveWithTimeout(MutableBufferSequence& buffers, size_t& nReceivedBytes,
                          const boost::asio::deadline_timer::duration_type& expiry_time);
  /** Receives data until specified length is completely received.
   * Received data are stored in the data buffer.
   * @param[in] data uint8_t buffer where received data will be stored
   * @param[in] length length of data to be received
   * @return received size
   */
  inline size_t receiveLoopUntilSpecifiedLengthCompletes(uint8_t* data, uint32_t length);
  /** Receives data. The maximum length can be specified as length.
   * Receive size may be shorter than the specified length.
   * @param[in] data uint8_t buffer where received data will be stored
   * @param[in] length the maximum size of the data buffer
   * @return received size
   */
  size_t receive(uint8_t* data, uint32_t length, bool waitUntilSpecifiedLengthCompletes = false);
  void setTimeout(double timeoutDurationInMilliSec);

 private:
  std::unique_ptr<boost::asio::serial_port> port_;
  boost::asio::io_service io_{};
  boost::asio::streambuf receiveBuffer_{};
  std::vector<uint8_t> internalSendBuffer_{};
  std::mutex mutex_{};
  double timeoutDurationInMilliSec_ = 1000.0;
  boost::posix_time::millisec timeoutDurationObject_;
};

#endif /* SERIALPORT_HH_ */
