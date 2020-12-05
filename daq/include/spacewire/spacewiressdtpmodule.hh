#ifndef SPACEWIRE_SPACEWIRESSDTPMODULE_HH_
#define SPACEWIRE_SPACEWIRESSDTPMODULE_HH_

#include <mutex>

#include "spacewire/tcpsocket.hh"

/** An exception class used by SpaceWireSSDTPModule.
 */
class SpaceWireSSDTPException : public Exception {
 public:
  enum {
    DataSizeTooLarge,
    Disconnected,
    ReceiveFailed,
    SendFailed,
    Timeout,
    TCPSocketError,
    EEP,
    SequenceError,
    NotImplemented,
    TimecodeReceiveError,
    Undefined
  };

  SpaceWireSSDTPException(u32 status) : Exception(status) {}
  ~SpaceWireSSDTPException() override = default;
  std::string toString() const override {
    std::string result;
    switch (status_) {
      case DataSizeTooLarge:
        result = "DataSizeTooLarge";
        break;
      case Disconnected:
        result = "Disconnected";
        break;
      case ReceiveFailed:
        result = ReceiveFailed;
        break;
      case SendFailed:
        result = "SendFailed";
        break;
      case Timeout:
        result = "Timeout";
        break;
      case TCPSocketError:
        result = "TCPSocketError";
        break;
      case EEP:
        result = "EEP";
        break;
      case SequenceError:
        result = "SequenceError";
        break;
      case NotImplemented:
        result = "NotImplemented";
        break;
      case TimecodeReceiveError:
        result = "TimecodeReceiveError";
        break;
      case Undefined:
        result = "Undefined";
        break;

      default:
        result = "Undefined status";
        break;
    }
    return result;
  }
};

/** A class that performs synchronous data transfer via
 * TCP/IP network using "Simple- Synchronous- Data Transfer Protocol"
 * which is defined for this class.
 */
class SpaceWireSSDTPModule {
 public:
  SpaceWireSSDTPModule(TCPSocket* newdatasocket) {
    datasocket = newdatasocket;
    sendbuffer = (u8*)malloc(SpaceWireSSDTPModule::BufferSize);
    receivebuffer = (u8*)malloc(SpaceWireSSDTPModule::BufferSize);
  }

  ~SpaceWireSSDTPModule() {
    delete sendbuffer;
    delete receivebuffer;
  }

  /** Sends a SpaceWire packet via the SpaceWire interface.
   * This is a blocking method.
   * @param[in] data packet content.
   * @param[in] eopType End-of-Packet marker. EOPType::EOP or EOPType::EEP.
   */
  void send(std::vector<u8>* data, EOPType eopType = EOPType::EOP) {
    std::lock_guard<std::mutex> guard(sendmutex);
    if (this->closed) {
      return;
    }
    size_t size = data->size();
    if (eopType == EOPType ::EOP) {
      sheader[0] = DataFlag_Complete_EOP;
    } else if (eopType == EOPType ::EEP) {
      sheader[0] = DataFlag_Complete_EEP;
    } else if (eopType == EOPType ::Continued) {
      sheader[0] = DataFlag_Flagmented;
    }
    sheader[1] = 0x00;
    for (size_t i = 11; i > 1; i--) {
      sheader[i] = size % 0x100;
      size = size / 0x100;
    }
    try {
      datasocket->send(sheader.data(), 12);
      datasocket->send(data->data(), data->size());
    } catch (...) {
      throw SpaceWireSSDTPException(SpaceWireSSDTPException::Disconnected);
    }
  }

  /** Sends a SpaceWire packet via the SpaceWire interface.
   * This is a blocking method.
   * @param[in] data packet content.
   * @param[in] the length length of the packet.
   * @param[in] eopType End-of-Packet marker. EOPType::EOP or EOPType::EEP.
   */
  void send(const u8* data, size_t length, EOPType eopType = EOPType::EOP) {
    std::lock_guard<std::mutex> guard(sendmutex);
    if (this->closed) {
      return;
    }
    if (eopType == EOPType::EOP) {
      sheader[0] = DataFlag_Complete_EOP;
    } else if (eopType == EOPType::EEP) {
      sheader[0] = DataFlag_Complete_EEP;
    } else if (eopType == EOPType::Continued) {
      sheader[0] = DataFlag_Flagmented;
    }
    sheader[1] = 0x00;
    size_t asize = length;
    for (size_t i = 11; i > 1; i--) {
      sheader[i] = asize % 0x100;
      asize = asize / 0x100;
    }
    try {
      datasocket->send(sheader.data(), 12);
      datasocket->send(data, length);
    } catch (...) {
      throw SpaceWireSSDTPException(SpaceWireSSDTPException::Disconnected);
    }
  }

  /** Tries to receive a pcket from the SpaceWire interface.
   * This method will block the thread for a certain length of time.
   * Timeout can happen via the TCPSocket provided to the instance.
   * The code below shows how the timeout duration can be changed.
   * @code
   * TCPClientSocket* socket=new TCPClientSocket("192.168.1.100", 10030);
   * socket->open();
   * socket->setTimeoutDuration(1000); //ms
   * SpaceWireSSDTPModule* ssdtpModule=new SpaceWireSSDTPModule(socket);
   * try{
   * 	ssdtpModule->receive();
   * }catch(SpaceWireSSDTPException& e){
   * 	if(e.getStatus()==SpaceWireSSDTPException::Timeout){
   * 	 std::cout << "Timeout" << std::endl;
   * 	}else{
   * 	 throw e;
   * 	}
   * }
   * @endcode
   * @returns packet content.
   */
  std::vector<u8> receive() {
    std::lock_guard<std::mutex> guard(receivemutex);
    if (this->closed) {
      return {};
    }
    std::vector<u8> data;
    EOPType eopType{};
    receive(&data, eopType);
    return data;
  }

  /** Tries to receive a pcket from the SpaceWire interface.
   * This method will block the thread for a certain length of time.
   * Timeout can happen via the TCPSocket provided to the instance.
   * The code below shows how the timeout duration can be changed.
   * @code
   * TCPClientSocket* socket=new TCPClientSocket("192.168.1.100", 10030);
   * socket->open();
   * socket->setTimeoutDuration(1000); //ms
   * SpaceWireSSDTPModule* ssdtpModule=new SpaceWireSSDTPModule(socket);
   * try{
   * 	ssdtpModule->receive();
   * }catch(SpaceWireSSDTPException& e){
   * 	if(e.getStatus()==SpaceWireSSDTPException::Timeout){
   * 	 std::cout << "Timeout" << std::endl;
   * 	}else{
   * 	 throw e;
   * 	}
   * }
   * @endcode
   * @param[out] data a vector instance which is used to store received data.
   * @param[out] eopType contains an EOP marker type (EOPType::EOP or EOPType::EEP).
   */
  size_t receive(std::vector<u8>* data, EOPType& eopType) {
    size_t size = 0;
    size_t hsize = 0;
    size_t flagment_size = 0;
    size_t received_size = 0;

    try {
      std::lock_guard<std::mutex> guard(receivemutex);
    // header
    receive_header:  //
      rheader[0] = 0xFF;
      rheader[1] = 0x00;
      while (rheader[0] != DataFlag_Complete_EOP && rheader[0] != DataFlag_Complete_EEP) {
        hsize = 0;
        flagment_size = 0;
        received_size = 0;
        // flag and size part
        try {
          while (hsize != 12) {
            if (this->closed) {
              return 0;
            }
            if (this->receiveCanceled) {
              // reset receiveCanceled
              this->receiveCanceled = false;
              // return with no data
              return 0;
            }
            const auto result = datasocket->receive(rheader.data() + hsize, 12 - hsize);
            hsize += result;
          }
        } catch (const TCPSocketException& e) {
          if (e.getStatus() == TCPSocketException::Timeout) {
            throw SpaceWireSSDTPException(SpaceWireSSDTPException::Timeout);
          } else {
            throw SpaceWireSSDTPException(SpaceWireSSDTPException::Disconnected);
          }
        } catch (...) {
          throw SpaceWireSSDTPException(SpaceWireSSDTPException::Disconnected);
        }

        // data or control code part
        if (rheader[0] == DataFlag_Complete_EOP || rheader[0] == DataFlag_Complete_EEP ||
            rheader[0] == DataFlag_Flagmented) {
          // data
          for (u32 i = 2; i < 12; i++) {
            flagment_size = flagment_size * 0x100 + rheader[i];
          }
          u8* data_pointer = receivebuffer;
          while (received_size != flagment_size) {
            long result;
          _loop_receiveDataPart:  //
            try {
              result = datasocket->receive(data_pointer + size + received_size, flagment_size - received_size);
            } catch (const TCPSocketException& e) {
              if (e.getStatus() == TCPSocketException::Timeout) {
                goto _loop_receiveDataPart;
              }
              // TODO: error handling
              exit(-1);
            }
            received_size += result;
          }
          size += received_size;
        } else if (rheader[0] == ControlFlag_SendTimeCode || rheader[0] == ControlFlag_GotTimeCode) {
          // control
          u8 timecode_and_reserved[2];
          u32 tmp_size = 0;
          try {
            while (tmp_size != 2) {
              int result = datasocket->receive(timecode_and_reserved + tmp_size, 2 - tmp_size);
              tmp_size += result;
            }
          } catch (...) {
            throw SpaceWireSSDTPException(SpaceWireSSDTPException::TCPSocketError);
          }
        } else {
          throw SpaceWireSSDTPException(SpaceWireSSDTPException::TCPSocketError);
        }
      }
      data->resize(size);
      if (size != 0) {
        memcpy(&(data->at(0)), receivebuffer, size);
      } else {
        goto receive_header;
      }
      if (rheader[0] == DataFlag_Complete_EOP) {
        eopType = EOPType::EOP;
      } else if (rheader[0] == DataFlag_Complete_EEP) {
        eopType = EOPType::EEP;
      } else {
        eopType = EOPType::Continued;
      }

      return size;
    } catch (SpaceWireSSDTPException& e) {
      throw e;
    } catch (TCPSocketException& e) {
      throw SpaceWireSSDTPException(SpaceWireSSDTPException::TCPSocketError);
    }
  }

  void close() { this->closed = true; }
  void cancelReceive() { this->receiveCanceled = true; }

 public:
  /* for SSDTP2 */
  static const u8 DataFlag_Complete_EOP = 0x00;
  static const u8 DataFlag_Complete_EEP = 0x01;
  static const u8 DataFlag_Flagmented = 0x02;
  static const u8 ControlFlag_SendTimeCode = 0x30;
  static const u8 ControlFlag_GotTimeCode = 0x31;
  static const u8 ControlFlag_ChangeTxSpeed = 0x38;
  static const u8 ControlFlag_RegisterAccess_ReadCommand = 0x40;
  static const u8 ControlFlag_RegisterAccess_ReadReply = 0x41;
  static const u8 ControlFlag_RegisterAccess_WriteCommand = 0x50;
  static const u8 ControlFlag_RegisterAccess_WriteReply = 0x51;
  static const u32 LengthOfSizePart = 10;

  static const u32 BufferSize = 10 * 1024 * 1024;

 private:
  size_t receivedsize{};
  size_t rbuf_index{};

  bool closed = false;
  bool receiveCanceled = false;

  TCPSocket* datasocket{};
  u8* sendbuffer{};
  u8* receivebuffer{};
  std::stringstream ss;
  u32 latest_sentsize{};
  std::mutex sendmutex;
  std::mutex receivemutex;

  std::array<u8, 12> rheader{};
  std::array<u8, 30> r_tmp{};
  std::array<u8, 12> sheader{};
};

#endif
