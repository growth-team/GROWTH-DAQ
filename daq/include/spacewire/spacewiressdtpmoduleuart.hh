#ifndef SPACEWIRE_SPACEWIRESSDTPMODULEUART_HH_
#define SPACEWIRE_SPACEWIRESSDTPMODULEUART_HH_

#include <array>
#include <mutex>
#include <memory>

#include "spdlog/spdlog.h"

#include "spacewire/serialport.hh"
#include "spacewire/spacewireif.hh"
#include "spacewire/spacewiressdtpmodule.hh"
#include "types.h"

/** A class that performs synchronous data transfer via
 * UART using "Simple- Synchronous- Data Transfer Protocol".
 */
class SpaceWireSSDTPModuleUART {
 public:
  using SerialPortPtr =std::unique_ptr<SerialPort>;
  SpaceWireSSDTPModuleUART(SerialPortPtr serialPort);
  ~SpaceWireSSDTPModuleUART() = default;

  void send(const u8* data, size_t length, EOPType eopType = EOPType::EOP);
  size_t receive(std::vector<u8>* receiveBuffer, EOPType& eopType);
  void sendRawData(const u8* data, size_t length);
  void close() { closed_ = true; }
  void cancelReceive() { receiveCanceled_ = true; }

 private:
  SerialPortPtr serialPort_;
  std::vector<u8> sendBuffer_{};
  u8 internalTimecode_{};
  u32 latestSendSize_{};
  std::mutex sendMutex_;
  std::mutex receiveMutex_;
  bool atLeastOnePacketSuccessfulyReceived_ = false;
  bool closed_ = false;
  std::atomic<bool> receiveCanceled_{false};

  static constexpr size_t HEADER_LENGTH_BYTES = 12;
  std::array<u8, HEADER_LENGTH_BYTES> rheader_{};
  std::array<u8, HEADER_LENGTH_BYTES> sheader_{};

 public:
  static constexpr u32 MAX_RECEIVE_SIZE_BYTES = 100 * 1024;

  /* for SSDTP2 */
  static constexpr u8 DataFlag_Complete_EOP = 0x00;
  static constexpr u8 DataFlag_Complete_EEP = 0x01;
  static constexpr u8 DataFlag_Flagmented = 0x02;
  static constexpr u8 ControlFlag_SendTimeCode = 0x30;
  static constexpr u8 ControlFlag_GotTimeCode = 0x31;
  static constexpr u8 ControlFlag_ChangeTxSpeed = 0x38;
  static constexpr u8 ControlFlag_RegisterAccess_ReadCommand = 0x40;
  static constexpr u8 ControlFlag_RegisterAccess_ReadReply = 0x41;
  static constexpr u8 ControlFlag_RegisterAccess_WriteCommand = 0x50;
  static constexpr u8 ControlFlag_RegisterAccess_WriteReply = 0x51;
  static constexpr u32 LengthOfSizePart = 10;
};

#endif
