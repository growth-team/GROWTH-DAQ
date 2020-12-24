#ifndef GROWTH_FPGA_CONSUMERMANAGEREVENTFIFO_HH_
#define GROWTH_FPGA_CONSUMERMANAGEREVENTFIFO_HH_

#include "types.h"
#include "growth-fpga/registeraccessinterface.hh"

#include <cassert>

class RMAPHandlerUART;

/** A class which represents ConsumerManager module in the VHDL logic.
 * It also holds information on a ring buffer constructed on SDRAM.
 */
class ConsumerManagerEventFIFO : public RegisterAccessInterface {
 public:
  /** Constructor.
   * @param[in] rmapHandler RMAPHandlerUART
   * @param[in] adcRMAPTargetNode RMAPTargetNode that corresponds to the ADC board
   */
  ConsumerManagerEventFIFO(std::shared_ptr<RMAPHandlerUART> rmapHandler, std::shared_ptr<RMAPTargetNode> rmapTargetNode)
      : RegisterAccessInterface(rmapHandler, rmapTargetNode) {}
  ~ConsumerManagerEventFIFO() override = default;

  void enableEventOutput() {
    constexpr u16 disableFlag = 0;
    write(AddressOf_EventOutputDisableRegister, disableFlag);
    spdlog::debug("ConsumerManager::enableEventOutput() disable register readback = {}",
                  read16(AddressOf_EventOutputDisableRegister));
  }

  void disableEventOutput() {
    constexpr u16 disableFlag = 0xFFFF;
    write(AddressOf_EventOutputDisableRegister, disableFlag);
    spdlog::debug("ConsumerManager::enableEventOutput() disable register readback = {}",
                  read16(AddressOf_EventOutputDisableRegister));
  }

  /** Retrieve data stored in the EventFIFO.
   * @param maxBytes maximum data size to be returned (in bytes)
   */
  const std::vector<u8>& getEventData(size_t maxBytes = 4000) {
    assert(maxBytes <= RECEIVE_BUFFER_SIZE_BYTES);
    receiveBuffer_.resize(RECEIVE_BUFFER_SIZE_BYTES);
    try {
      size_t receivedSize = this->readEventFIFO(&(receiveBuffer_[0]), RECEIVE_BUFFER_SIZE_BYTES);
      // if odd byte is received, wait until the following 1 byte is received.
      if (receivedSize % 2 == 1) {
        spdlog::warn("ConsumerManagerEventFIFO::getEventData(): odd bytes. wait for another byte.");
        size_t receivedSizeOneByte = 0;
        while (receivedSizeOneByte == 0) {
          try {
            receivedSizeOneByte = this->readEventFIFO(&(receiveBuffer_[receivedSize]), 1);
          } catch (...) {
            spdlog::warn("ConsumerManagerEventFIFO::getEventData(): receive 1 byte timeout. continues.");
          }
        }
        // increment by 1 to make receivedSize even
        receivedSize++;
      }
      receiveBuffer_.resize(receivedSize);
    } catch (RMAPInitiatorException& e) {
      if (e.getStatus() == RMAPInitiatorException::Timeout) {
        spdlog::error("ConsumerManagerEventFIFO::getEventData(): timeout");
        receiveBuffer_.resize(0);
      } else {
        spdlog::error("ConsumerManagerEventFIFO::getEventData(): TCPSocketException on receive() ({})", e.toString());
        throw e;
      }
    }

    // return result
    receivedBytes_ += receiveBuffer_.size();
    return receiveBuffer_;
  }

  /** Sets EventPacket_NumberOfWaveform_Register
   * @param nSamples number of data points to be recorded in an event packet
   */
  void setEventPacket_NumberOfWaveform(u16 nSamples) {
    write(AddressOf_EventPacket_NumberOfWaveform_Register, nSamples);
    spdlog::debug("setEventPacket_NumberOfWaveform write value = {:d}, readback = {:d}", nSamples,
                  read16(AddressOf_EventPacket_NumberOfWaveform_Register));
  }

 private:
  size_t readEventFIFO(u8* buffer, size_t length) {
    const size_t dataCountsInWords = read16(AddressOf_EventFIFO_DataCount_Register);
    const size_t dataCountInBytes = dataCountsInWords * sizeof(u16);
    const size_t readSize = std::min(dataCountInBytes, length);
    if (readSize != 0) {
      read(InitialAddressOf_EventFIFO, readSize, buffer);
    }
    return readSize;
  }

  // Addresses of Consumer Manager Module
  static constexpr u32 InitialAddressOf_ConsumerMgr = 0x01010000;
  static constexpr u32 ConsumerMgrBA = InitialAddressOf_ConsumerMgr;
  static constexpr u32 AddressOf_EventOutputDisableRegister = ConsumerMgrBA + 0x0100;
  static constexpr u32 AddressOf_GateSize_FastGate_Register = ConsumerMgrBA + 0x010e;
  static constexpr u32 AddressOf_GateSize_SlowGate_Register = ConsumerMgrBA + 0x0110;
  static constexpr u32 AddressOf_NumberOf_BaselineSample_Register = ConsumerMgrBA + 0x0112;
  static constexpr u32 AddressOf_ConsumerMgr_ResetRegister = ConsumerMgrBA + 0x0114;
  static constexpr u32 AddressOf_EventPacket_NumberOfWaveform_Register = ConsumerMgrBA + 0x0116;
  static constexpr u32 AddressOf_EventPacket_WaveformDownSampling_Register = ConsumerMgrBA + 0xFFFF;

  // Addresses of EventFIFO
  static constexpr u32 InitialAddressOf_EventFIFO = 0x10000000;
  static constexpr u32 FinalAddressOf_EventFIFO = 0x1000FFFF;
  static constexpr u32 AddressOf_EventFIFO_DataCount_Register = 0x20000000;

  static constexpr size_t RECEIVE_BUFFER_SIZE_BYTES = 4096 * 3;
  static constexpr size_t EVENT_FIFIO_SIZE_BYTES = 2 * 16 * 1024;  // 16-bit wide * 16-k depth

  std::vector<u8> receiveBuffer_{};
  size_t receivedBytes_ = 0;
};

#endif
