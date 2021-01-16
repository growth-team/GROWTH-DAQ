#ifndef SPACEWIRE_SPACEWIRESSDTPMODULEUART_HH_
#define SPACEWIRE_SPACEWIRESSDTPMODULEUART_HH_

#include <array>
#include <exception>
#include <mutex>

#include "spacewire/serialport.hh"
#include "spacewire/spacewireif.hh"
#include "spacewire/spacewiressdtpmodule.hh"
#include "types.h"

/** A class that performs synchronous data transfer via
 * UART using "Simple- Synchronous- Data Transfer Protocol".
 */
class SpaceWireSSDTPModuleUART {
 public:
  SpaceWireSSDTPModuleUART(SerialPort* serialPort)
      : serialPort_(serialPort), sendBuffer_(BufferSize), receiveBuffer_(BufferSize) {
    if (!serialPort) {
      throw std::runtime_error("serialPort pointer must not be nullptr");
    }
  }

  ~SpaceWireSSDTPModuleUART() = default;

  void send(const u8* data, size_t length, EOPType eopType = EOPType::EOP) {
    std::lock_guard<std::mutex> guard(sendMutex_);
    if (closed_) {
      return;
    }
    if (eopType == EOPType::EOP) {
      sheader_[0] = DataFlag_Complete_EOP;
    } else if (eopType == EOPType::EEP) {
      sheader_[0] = DataFlag_Complete_EEP;
    } else if (eopType == EOPType::Continued) {
      sheader_[0] = DataFlag_Flagmented;
    }
    sheader_[1] = 0x00;
    size_t asize = length;
    for (size_t i = 11; i > 1; i--) {
      sheader_[i] = asize % 0x100;
      asize = asize / 0x100;
    }
    try {
      serialPort_->send(sheader_.data(), 12);
      serialPort_->send(data, length);
    } catch (...) {
      throw SpaceWireSSDTPException(SpaceWireSSDTPException::Disconnected);
    }
  }

  size_t receive(std::vector<u8>* data, EOPType& eopType) {
    size_t size = 0;
    size_t hsize = 0;
    size_t flagment_size = 0;
    size_t received_size = 0;

    data->clear();
    receiveCanceled_ = false;

    try {
      std::lock_guard<std::mutex> guard(receiveMutex_);
    // header
    receive_header:  //
      rheader_[0] = 0xFF;
      rheader_[1] = 0x00;
      while (rheader_[0] != DataFlag_Complete_EOP && rheader_[0] != DataFlag_Complete_EEP) {
        hsize = 0;
        flagment_size = 0;
        received_size = 0;
        // flag and size part
        try {
          while (hsize != 12) {
            if (closed_) {
              throw SpaceWireSSDTPException(SpaceWireSSDTPException::Disconnected);
            }
            if (receiveCanceled_) {
              return 0;
            }
            const size_t result = serialPort_->receive(rheader_.data() + hsize, 12 - hsize);
            hsize += result;
          }
        } catch (const SerialPortException& e) {
          throw SpaceWireSSDTPException(SpaceWireSSDTPException::Timeout);
        } catch (...) {
          throw SpaceWireSSDTPException(SpaceWireSSDTPException::Disconnected);
        }
        u8* data_pointer = receiveBuffer_.data();

        // data or control code part
        if (rheader_[0] == DataFlag_Complete_EOP || rheader_[0] == DataFlag_Complete_EEP ||
            rheader_[0] == DataFlag_Flagmented) {
          // data
          for (size_t i = 2; i < 12; i++) {
            flagment_size = flagment_size * 0x100 + rheader_[i];
          }
          // verify fragment size
          if (flagment_size > BufferSize) {
            throw SpaceWireSSDTPException(SpaceWireSSDTPException::DataSizeTooLarge);
          }

          while (received_size != flagment_size) {
            long result;
          _loop_receiveDataPart:  //
            if (receiveCanceled_) {
              return 0;
            }
            try {
              result = serialPort_->receive(data_pointer + size + received_size, flagment_size - received_size);
            } catch (const SerialPortException& e) {
              throw SpaceWireSSDTPException(SpaceWireSSDTPException::Timeout);
            }
            received_size += result;
          }
          size += received_size;
        } else if (rheader_[0] == ControlFlag_SendTimeCode || rheader_[0] == ControlFlag_GotTimeCode) {
          std::array<u8, 2> timecodeReceiveBuffer{};
          size_t receivedSize = 0;
          try {
            while (receivedSize < 2) {
              int result = serialPort_->receive(timecodeReceiveBuffer.data() + receivedSize, 2 - receivedSize);
              receivedSize += result;
            }
          } catch (const SerialPortException& e) {
            throw SpaceWireSSDTPException(SpaceWireSSDTPException::TimecodeReceiveError);
          }
        } else {
          throw SpaceWireSSDTPException(SpaceWireSSDTPException::ReceiveFailed);
        }
      }
      data->resize(size);
      if (size != 0) {
        memcpy(data->data(), receiveBuffer_.data(), size);
      } else {
        goto receive_header;
      }
      if (rheader_[0] == DataFlag_Complete_EOP) {
        eopType = EOPType::EOP;
      } else if (rheader_[0] == DataFlag_Complete_EEP) {
        eopType = EOPType::EEP;
      } else {
        eopType = EOPType::Continued;
      }

      return size;
    } catch (const SpaceWireSSDTPException& e) {
      throw e;
    } catch (const SerialPortException& e) {
      throw SpaceWireSSDTPException(SpaceWireSSDTPException::Timeout);
    }
  }

  void sendRawData(const u8* data, size_t length) {
    std::lock_guard<std::mutex> guard(sendMutex_);
    try {
      serialPort_->send(data, length);
    } catch (const SerialPortException& e) {
      throw SpaceWireSSDTPException(SpaceWireSSDTPException::Timeout);
    }
  }
  void close() { closed_ = true; }
  void cancelReceive() { receiveCanceled_ = true; }

 private:
  SerialPort* serialPort_;
  std::vector<u8> sendBuffer_{};
  std::vector<u8> receiveBuffer_{};
  u8 internalTimecode_{};
  u32 latestSendSize_{};
  std::mutex sendMutex_;
  std::mutex receiveMutex_;

  bool closed_ = false;
  std::atomic<bool> receiveCanceled_{false};

  std::array<u8, 12> rheader_{};
  std::array<u8, 12> sheader_{};

 public:
  static constexpr u32 BufferSize = 100 * 1024;

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
