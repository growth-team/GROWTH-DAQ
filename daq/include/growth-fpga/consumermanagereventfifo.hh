#ifndef GROWTH_FPGA_CONSUMERMANAGEREVENTFIFO_HH_
#define GROWTH_FPGA_CONSUMERMANAGEREVENTFIFO_HH_

#include "types.h"
#include "growth-fpga/registeraccessinterface.hh"

#include <thread>

/** A class which represents ConsumerManager module in the VHDL logic.
 * It also holds information on a ring buffer constructed on SDRAM.
 */
class ConsumerManagerEventFIFO : public RegisterAccessInterface {
 public:
  ConsumerManagerEventFIFO(RMAPInitiatorPtr rmapInitiator, std::shared_ptr<RMAPTargetNode> rmapTargetNode);
  ~ConsumerManagerEventFIFO() override;

  void enableEventOutput();
  void disableEventOutput();

  /** Retrieve data stored in the EventFIFO.
   * @param maxBytes maximum data size to be returned (in bytes)
   */
  void getEventData(std::vector<u8>& receiveBuffer);

  /** Sets EventPacket_NumberOfWaveform_Register
   * @param nSamples number of data points to be recorded in an event packet
   */
  void setEventPacket_NumberOfWaveform(u16 nSamples);

 private:
  size_t readEventFIFO(u8* buffer, size_t length);

  // Addresses of Consumer Manager Module
  static constexpr u32 BASE_ADDRESS = 0x01010000;
  static constexpr u32 ADDRESS_EVENT_OUTPUT_DISABLE_REGISTER = BASE_ADDRESS + 0x0100;
  static constexpr u32 ADDRESS_GATE_SIZE_FAST_GATE_REGISTER = BASE_ADDRESS + 0x010e;
  static constexpr u32 ADDRESS_GATE_SIZE_SLOW_GATE_REGISTER = BASE_ADDRESS + 0x0110;
  static constexpr u32 ADDRESS_NUMBER_OF_BASELINE_SAMPLE_REGISTER = BASE_ADDRESS + 0x0112;
  static constexpr u32 ADDRESS_CONSUMER_MGR_RESET_REGISTER = BASE_ADDRESS + 0x0114;
  static constexpr u32 ADDRESS_EVENT_PACKET_NUMBER_OF_WAVEFORM_REGISTER = BASE_ADDRESS + 0x0116;
  static constexpr u32 ADDRESS_EVENT_PACKET_WAVEFORM_DOWN_SAMPLING_REGISTER = BASE_ADDRESS + 0xFFFF;

  // Addresses of EventFIFO
  static constexpr u32 INITIAL_ADDRESS_EVENT_FIFO = 0x10000000;
  static constexpr u32 FINAL_ADDRESS_EVENT_FIFO = 0x1000FFFF;
  static constexpr u32 ADDRESS_EVENT_FIFO_DATA_COUNT_REGISTER= 0x20000000;

  static constexpr size_t RECEIVE_BUFFER_SIZE_BYTES = 1024 * 32;
  static constexpr size_t SINGLE_READ_MAX_SIZE_BYTES = 4096 * 16;  // 4096 * 2;

  static constexpr size_t NUM_RECEIVE_BUFFERS = 1024;
  static constexpr size_t EVENT_FIFIO_SIZE_BYTES = 2 * 16 * 1024;  // 16-bit wide * 16-k depth

  size_t receivedBytes_ = 0;

  bool eventDataReadLoopStarted_ = false;
  bool eventDataReadLoopStop_ = false;
  std::thread eventDataReadLoopThread_;
};

#endif
