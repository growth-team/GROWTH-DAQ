#ifndef DAQ_SERIALPORT_HH_
#define DAQ_SERIALPORT_HH_

#include "types.h"

#include <serial/serial.h>

#include <memory>
#include <mutex>
#include <stdexcept>
#include <vector>

class SerialPortTimeoutException : public std::runtime_error {};

struct SerialPort {
 public:
  static constexpr size_t ReadBufferSize = 10 * 1024;

 public:
  SerialPort(const std::string& deviceName, size_t baudRate = 9600);
  void close();
  void send(const u8* buffer, size_t length);
  size_t receive(u8* buffer, size_t length);
  /** Receives data until specified length is completely received.
   * Received data are stored in the data buffer.
   * @param[in] data u8 buffer where received data will be stored
   * @param[in] length length of data to be received
   * @return received size
   */
  inline size_t receiveLoopUntilSpecifiedLengthCompletes(u8* data, u32 length);
  /** Receives data. The maximum length can be specified as length.
   * Receive size may be shorter than the specified length.
   * @param[in] data u8 buffer where received data will be stored
   * @param[in] length the maximum size of the data buffer
   * @return received size
   */
  size_t receive(u8* data, u32 length, bool waitUntilSpecifiedLengthCompletes = false);

 private:
  std::unique_ptr<serial::Serial> serial_;
  std::vector<u8> internalSendBuffer_{};
  std::mutex mutex_{};
};

#endif /* DAQ_SERIALPORT_HH_ */
