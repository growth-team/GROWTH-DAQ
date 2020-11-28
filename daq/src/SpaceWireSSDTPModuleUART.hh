#ifndef SPACEWIRESSDTPMODULEUART_HH_
#define SPACEWIRESSDTPMODULEUART_HH_

#include <array>
#include <mutex>

#include "SerialPort.hh"
#include "spacewire/spacewireif.hh"
#include "types.h"

/** A class that performs synchronous data transfer via
 * UART using "Simple- Synchronous- Data Transfer Protocol".
 */
class SpaceWireSSDTPModuleUART {
 public:
  /** Constructor. */
  SpaceWireSSDTPModuleUART(SerialPort* serialPort)
      : serialPort_(serialPort),
        sendBuffer_(SpaceWireSSDTPModule::BufferSize, 0),
        receiveBuffer_(SpaceWireSSDTPModule::BufferSize, 0) {}

  ~SpaceWireSSDTPModuleUART() = default;

 public:
  /** Sends a SpaceWire packet via the SpaceWire interface.
   * This is a blocking method.
   * @param[in] data packet content.
   * @param[in] eopType End-of-Packet marker. SpaceWireEOPMarker::EOP or SpaceWireEOPMarker::EEP.
   */
  void send(std::vector<u8>* data, u32 eopType = SpaceWireEOPMarker::EOP) {
    send(data->data(), eopType);
  }

 public:
  /** Sends a SpaceWire packet via the SpaceWire interface.
   * This is a blocking method.
   * @param[in] data packet content.
   * @param[in] the length length of the packet.
   * @param[in] eopType End-of-Packet marker. SpaceWireEOPMarker::EOP or SpaceWireEOPMarker::EEP.
   */
  void send(u8* data, size_t length, u32 eopType = SpaceWireEOPMarker::EOP) {
    std::lock_guard<std::mutex> guard(sendMutex_);
    if (closed_) {
      return;
    }
    if (eopType == SpaceWireEOPMarker::EOP) {
      sheader_[0] = DataFlag_Complete_EOP;
    } else if (eopType == SpaceWireEOPMarker::EEP) {
      sheader_[0] = DataFlag_Complete_EEP;
    } else if (eopType == SpaceWireEOPMarker::Continued) {
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
#ifdef DEBUG_SSDTP
      using namespace std;
      cout << "SSDTP::send():" << endl;
      cout << "Header: ";
      for (size_t i = 0; i < 12; i++) {
        cout << hex << right << setw(2) << setfill('0') << static_cast<u32>(sheader_[i]) << " ";
      }
      cout << endl;
      cout << "Data: " << dec << length << " bytes" << endl;
      for (size_t i = 0; i < length; i++) {
        cout << hex << right << setw(2) << setfill('0') << static_cast<u32>(data[i]) << " ";
      }
      cout << endl << dec;
#endif
    } catch (...) {
      throw SpaceWireSSDTPException(SpaceWireSSDTPException::Disconnected);
    }
  }

 public:
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
    std::lock_guard<std::mutex> guard(receiveMutex_);
    if (closed_) {
      return {};
    }
    std::vector<u8> data;
    u32 eopType{};
    receive(&data, eopType);
    return data;
  }

 public:
  /** Tries to receive a packet from the SpaceWire interface.
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
   * @param[out] eopType contains an EOP marker type (SpaceWireEOPMarker::EOP or SpaceWireEOPMarker::EEP).
   */
  int receive(std::vector<u8>* data, u32& eopType) {
    size_t size = 0;
    size_t hsize = 0;
    size_t flagment_size = 0;
    size_t received_size = 0;

    try {
      using namespace std;
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
              return 0;
            }
            if (receiveCanceled_) {
              // reset receiveCanceled
              receiveCanceled_ = false;
              // return with no data
              return 0;
            }
#ifdef DEBUG_SSDTP
            using namespace std;
            cout << "SSDTP::receive(): receiving header part (remaining size=" << dec << 12 - hsize << " bytes)"
                 << endl;
#endif
            const size_t result = serialPort_->receive(rheader_.data() + hsize, 12 - hsize);
            hsize += result;
          }
        } catch (SerialPortTimeoutException& e) {
#ifdef DEBUG_SSDTP
          using namespace std;
          cout << "SSDTP::receive(): serial port timed out." << endl;
#endif
          throw SpaceWireSSDTPException(SpaceWireSSDTPException::Timeout);
        } catch (...) {
          throw SpaceWireSSDTPException(SpaceWireSSDTPException::Disconnected);
        }

#ifdef DEBUG_SSDTP
        using namespace std;
        cout << "SSDTP::receive(): header part received" << endl;
        cout << "Header: ";
        for (size_t i = 0; i < 12; i++) {
          cout << hex << right << setw(2) << setfill('0') << static_cast<u32>(rheader_[i]) << " ";
        }
        cout << endl;
#endif
        u8* data_pointer = receiveBuffer_.data();

        // data or control code part
        if (rheader_[0] == DataFlag_Complete_EOP || rheader_[0] == DataFlag_Complete_EEP ||
            rheader_[0] == DataFlag_Flagmented) {
          // data
          for (u32 i = 2; i < 12; i++) {
            flagment_size = flagment_size * 0x100 + rheader_[i];
          }
          // verify fragment size
          if (flagment_size > BufferSize) {
            cerr << "SpaceWireSSDTPModuleUART::receive(): too large fragment size (" << flagment_size << " bytes)"
                 << endl;
            cerr << "Something is wrong with SSDTP over Serial Port data communication." << endl;
            cerr << "Trying to receive remaining data from receive buffer." << endl;
            size_t result = serialPort_->receive(data_pointer, BufferSize);
            cerr << result << " bytes received." << endl;
            cout << "Data: ";
            for (size_t i = 0; i < result; i++) {
              cout << hex << right << setw(2) << setfill('0') << static_cast<u32>(receiveBuffer_.at(i)) << " ";
            }
            cout << endl;
            // TODO: refactor this with a proper error handling code
            exit(-1);
          }

          while (received_size != flagment_size) {
            long result;
          _loop_receiveDataPart:  //
            if (receiveCanceled_) {
              // reset receiveCanceled
              receiveCanceled_ = false;
              // dump info
              cerr << "SpaceWireSSDTPModuleUART::receive(): canceled while waiting to receive " << dec << flagment_size
                   << " bytes." << endl;
              cout << "Current header:" << endl;
              for (size_t i = 0; i < 12; i++) {
                cout << hex << right << setw(2) << setfill('0') << static_cast<u32>(rheader_[i]) << " ";
              }
              cout << dec << endl;
              // return with no data
              return 0;
            }
            try {
              result = serialPort_->receive(data_pointer + size + received_size, flagment_size - received_size);
#ifdef DEBUG_SSDTP
              using namespace std;
              cout << "SSDTP::receive(): data part received" << endl;
              cout << "Data: ";
              for (size_t i = 0; i < result; i++) {
                cout << hex << right << setw(2) << setfill('0')
                     << static_cast<u32>(*(data_pointer + size + received_size + i)) << " ";
              }
              cout << endl;
#endif
            } catch (SerialPortTimeoutException e) {
              goto _loop_receiveDataPart;
              cout << "SSDTPModule::receive() exception when receiving data" << endl;
              cout << "rheader[0]=0x" << setw(2) << setfill('0') << hex << static_cast<u32>(rheader_[0]) << endl;
              cout << "rheader[1]=0x" << setw(2) << setfill('0') << hex << static_cast<u32>(rheader_[1]) << endl;
              cout << "size=" << dec << size << endl;
              cout << "flagment_size=" << dec << flagment_size << endl;
              cout << "received_size=" << dec << received_size << endl;
              exit(-1);
            }
            received_size += result;
          }
          size += received_size;
        } else if (rheader_[0] == ControlFlag_SendTimeCode || rheader_[0] == ControlFlag_GotTimeCode) {
          // control
          u8 timecode_and_reserved[2];
          u32 tmp_size = 0;
          try {
            while (tmp_size != 2) {
              int result = serialPort_->receive(timecode_and_reserved + tmp_size, 2 - tmp_size);
              tmp_size += result;
            }
          } catch (...) {
            throw SpaceWireSSDTPException(SpaceWireSSDTPException::TCPSocketError);
          }
          switch (rheader_[0]) {
            case ControlFlag_SendTimeCode:
              internalTimecode_ = timecode_and_reserved[0];
              gotTimeCode(internalTimecode_);
              break;
            case ControlFlag_GotTimeCode:
              internalTimecode_ = timecode_and_reserved[0];
              gotTimeCode(internalTimecode_);
              break;
          }
        } else {
          cout << "SSDTP fatal error with flag value of 0x" << hex << static_cast<u32>(rheader_[0]) << dec << endl;
          cout << "Previous data: (" << dec << receivedDataPrevious_.size() << " bytes)" << endl;
          for (size_t i = 0; i < receivedDataPrevious_.size(); i++) {
            cout << hex << right << setw(2) << setfill('0') << static_cast<u32>(receivedDataPrevious_[i]) << " ";
          }
          cout << dec << endl;
          cout << "Current header:" << endl;
          for (size_t i = 0; i < 12; i++) {
            cout << hex << right << setw(2) << setfill('0') << static_cast<u32>(rheader_[i]) << " ";
          }
          cout << dec << endl;
          throw SpaceWireSSDTPException(SpaceWireSSDTPException::TCPSocketError);
        }
      }
      data->resize(size);
      if (size != 0) {
        memcpy(data->data(), receiveBuffer_.data(), size);
      } else {
        goto receive_header;
      }
      if (rheader_[0] == DataFlag_Complete_EOP) {
        eopType = SpaceWireEOPMarker::EOP;
      } else if (rheader_[0] == DataFlag_Complete_EEP) {
        eopType = SpaceWireEOPMarker::EEP;
      } else {
        eopType = SpaceWireEOPMarker::Continued;
      }

// debug receive data
#ifdef DEBUG_SSDTP
      static constexpr size_t receivedDataPrevious_Max = 4096;
      receivedDataPrevious_.resize(std::min(size, receivedDataPrevious_Max));
      memcpy(receivedDataPrevious_.data(), receiveBuffer_, std::min(size, receivedDataPrevious_Max));
#endif
      return size;
    } catch (SpaceWireSSDTPException& e) {
      throw e;
    } catch (SerialPortTimeoutException& e) {
      throw SpaceWireSSDTPException(SpaceWireSSDTPException::TCPSocketError);
    }
  }

 public:
  /** Sends raw byte array via the socket.
   * @attention Users should not use this method.
   */
  void sendRawData(const u8* data, size_t length) {
    std::lock_guard<std::mutex> guard(sendMutex_);
    try {
      serialPort_->send(data, length);
    } catch (SerialPortTimeoutException& e) {
      throw SpaceWireSSDTPException(SpaceWireSSDTPException::Timeout);
    }
  }

 public:
  /** Finalizes the instance.
   * If another thread is waiting in receive(), it will return with 0 (meaning 0 byte received),
   * enabling the thread to stop.
   */
  void close() { closed_ = true; }

 public:
  /** Cancels ongoing receive() method if any exist.
   */
  void cancelReceive() { receiveCanceled_ = true; }

 private:
  std::shared_ptr<SerialPort> serialPort_;
  std::vector<u8> sendBuffer_;
  std::vector<u8> receiveBuffer_;
  u8 internalTimecode_{};
  u32 latestSendSize_{};
  std::mutex sendMutex_;
  std::mutex receiveMutex_;
  SpaceWireIFActionTimecodeScynchronizedAction* timecodeaction_{};
  bool closed_ = false;
  bool receiveCanceled_ = false;

  std::array<u8, 12> rheader_{};
  std::array<u8, 12> sheader_{};

  std::vector<u8> receivedDataPrevious_;

 public:
  static constexpr u32 BufferSize = 10 * 1024;

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
