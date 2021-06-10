#ifndef GROWTH_FPGA_CONSUMERMANAGEREVENTFIFO_HH_
#define GROWTH_FPGA_CONSUMERMANAGEREVENTFIFO_HH_

#include "types.h"
#include "growth-fpga/registeraccessinterface.hh"

#include <cassert>

/** A class which represents ConsumerManager module in the VHDL logic.
 * It also holds information on a ring buffer constructed on SDRAM.
 */
class ConsumerManagerEventFIFO : public RegisterAccessInterface {
 public:
  ConsumerManagerEventFIFO(RMAPInitiatorPtr rmapInitiator, std::shared_ptr<RMAPTargetNode> rmapTargetNode)
      : RegisterAccessInterface(std::move(rmapInitiator), rmapTargetNode) {}

  ~ConsumerManagerEventFIFO() override {
    if (eventDataReadLoopThread_.joinable()) {
      eventDataReadLoopStop_ = true;
      eventDataReadLoopThread_.join();
    }
  }

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
  void getEventData(std::vector<u8>& receiveBuffer) {
    receiveBuffer.resize(RECEIVE_BUFFER_SIZE_BYTES);
    const auto receivedSize = readEventFIFO(receiveBuffer.data(), RECEIVE_BUFFER_SIZE_BYTES);
    receiveBuffer.resize(receivedSize);
    receivedBytes_ += receivedSize;
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
    const size_t totalReadSizeInBytes = [&]() {
      const size_t dataCountsInWords = read16(AddressOf_EventFIFO_DataCount_Register);
      const size_t dataCountInBytes = dataCountsInWords * sizeof(u16);
      return std::min(length, dataCountInBytes);
    }();

    size_t receivedSizeInBytes = 0;
    while (receivedSizeInBytes < totalReadSizeInBytes) {
      const size_t remainingReadSizeInBytes = totalReadSizeInBytes - receivedSizeInBytes;
      const size_t singleReadSize = std::min(remainingReadSizeInBytes, SINGLE_READ_MAX_SIZE_BYTES);
      assert(singleReadSize != 0);
      read(InitialAddressOf_EventFIFO, singleReadSize, buffer + receivedSizeInBytes);
      receivedSizeInBytes += singleReadSize;
    }
    return totalReadSizeInBytes;
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

  static constexpr size_t RECEIVE_BUFFER_SIZE_BYTES = 1024 * 32;
  static constexpr size_t SINGLE_READ_MAX_SIZE_BYTES = 4096 * 16; // 4096 * 2;

  static constexpr size_t NUM_RECEIVE_BUFFERS = 1024;
  static constexpr size_t EVENT_FIFIO_SIZE_BYTES = 2 * 16 * 1024;  // 16-bit wide * 16-k depth

  size_t receivedBytes_ = 0;

  bool eventDataReadLoopStarted_ = false;
  bool eventDataReadLoopStop_ = false;
  std::thread eventDataReadLoopThread_;
};

#endif