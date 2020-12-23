#ifndef GROWTH_FPGA_CONSUMERMANAGER_HH_
#define GROWTH_FPGA_CONSUMERMANAGER_HH_

#include <cassert>
#include "registeraccessinterface.hh"
#include "semaphoreregister.hh"

class RMAPHandlerUART;

/** A class which represents ConsumerManager module in the VHDL logic.
 * It also holds information on a ring buffer constructed on SDRAM.
 */
class ConsumerManager : public RegisterAccessInterface {
 public:
  ConsumerManager(std::shared_ptr<RMAPHandlerUART> rmapHandler)
      : RegisterAccessInterface(rmapHandler),
        writePointerSemaphore_(rmapHandler, ConsumerManager::AddressOf_Writepointer_Semaphore_Register) {}
  ~ConsumerManager() override = default;

  /** Resets ConsumerManager module.
   */
  virtual void reset() {
    std::vector<u8> writedata;
    writedata.push_back(0x01);
    writedata.push_back(0x00);
    rmapHandler_->write(adcboxRMAPNode, AddressOf_ConsumerMgr_ResetRegister, &writedata[0], 2);
  }

  /** Sets NumberOfBaselineSample_Register
   * @param numberofsamples number of data points to be sampled
   */
  virtual void setNumberOfBaselineSamples(size_t nSamples) {
    std::vector<u8> writedata;
    writedata.push_back((u8)nSamples);
    writedata.push_back(0x00);
    rmapHandler_->write(adcboxRMAPNode, AddressOf_NumberOf_BaselineSample_Register, &writedata[0], 2);
  }

  /** Sets EventPacket_NumberOfWaveform_Register
   * @param nSamples number of data points to be recorded in an event packet
   */
  virtual void setEventPacket_NumberOfWaveform(size_t nSamples) {
    std::vector<u8> writedata;
    writedata.push_back(static_cast<u8>(nSamples << 24 >> 24));
    writedata.push_back(static_cast<u8>(nSamples << 16 >> 24));
    rmapHandler_->write(adcboxRMAPNode, AddressOf_EventPacket_NumberOfWaveform_Register, &writedata[0], 2);
  }

  // Addresses of Consumer Manager Module
  // clang-format off
  static constexpr u32 InitialAddressOf_ConsumerMgr                    = 0x01010000;
  static constexpr u32 ConsumerMgrBA                                   = InitialAddressOf_ConsumerMgr;
  static constexpr u32 AddressOf_DisableRegister                       = ConsumerMgrBA + 0x0100;
  static constexpr u32 AddressOf_AddressUpdateGoRegister               = ConsumerMgrBA + 0x010c;
  static constexpr u32 AddressOf_GateSize_FastGate_Register            = ConsumerMgrBA + 0x010e;
  static constexpr u32 AddressOf_GateSize_SlowGate_Register            = ConsumerMgrBA + 0x0110;
  static constexpr u32 AddressOf_NumberOf_BaselineSample_Register      = ConsumerMgrBA + 0x0112;
  static constexpr u32 AddressOf_ConsumerMgr_ResetRegister             = ConsumerMgrBA + 0x0114;
  static constexpr u32 AddressOf_EventPacket_NumberOfWaveform_Register = ConsumerMgrBA + 0x0116;
  static constexpr u32 AddressOf_Writepointer_Semaphore_Register       = ConsumerMgrBA + 0x0118;
  // Addresses of SDRAM
  static constexpr u32 InitialAddressOf_Sdram_EventList = 0x00000000;
  static constexpr u32 FinalAddressOf_Sdram_EventList   = 0x00fffffe;
  // clang-format on

 private:
  u32 incrementAddress(u32 address) {
    if (address == FinalAddressOf_Sdram_EventList) {
      address = InitialAddressOf_Sdram_EventList;
    } else {
      address += 2;
    }
    return address;
  }

  SemaphoreRegister writePointerSemaphore_;

  // Ring buffer
  u32 readPointer_{};
  u32 writePointer_{};
  u32 nextReadFrom_{};
  u32 guardBit_{};
};

#endif /* CONSUMERMANAGER_HH_ */
