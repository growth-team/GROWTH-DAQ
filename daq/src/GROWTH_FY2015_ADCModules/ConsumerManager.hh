#ifndef CONSUMERMANAGER_HH_
#define CONSUMERMANAGER_HH_

#include "GROWTH_FY2015_ADCModules/RMAPHandler.hh"
#include "GROWTH_FY2015_ADCModules/RegisterAccessInterface.hh"
#include "GROWTH_FY2015_ADCModules/SemaphoreRegister.hh"

/** A class which represents ConsumerManager module in the VHDL logic.
 * It also holds information on a ring buffer constructed on SDRAM.
 */
class ConsumerManager : public RegisterAccessInterface {
 public:
  ConsumerManager(std::shared_ptr<RMAPHandler> rmapHandler)
      : RegisterAccessInterface(rmapHandler),
        writePointerSemaphore_(rmapHandler, ConsumerManager::AddressOf_Writepointer_Semaphore_Register) {}
  ~ConsumerManager() override = default;

  /** Resets ConsumerManager module.
   */
  virtual void reset() {
    std::vector<uint8_t> writedata;
    writedata.push_back(0x01);
    writedata.push_back(0x00);
    rmapHandler_->write(adcboxRMAPNode, AddressOf_ConsumerMgr_ResetRegister, &writedata[0], 2);
  }

  /** Sets NumberOfBaselineSample_Register
   * @param numberofsamples number of data points to be sampled
   */
  virtual void setNumberOfBaselineSamples(size_t nSamples) {
    vector<uint8_t> writedata;
    writedata.push_back((uint8_t)nSamples);
    writedata.push_back(0x00);
    rmapHandler_->write(adcboxRMAPNode, AddressOf_NumberOf_BaselineSample_Register, &writedata[0], 2);
  }

  /** Sets EventPacket_NumberOfWaveform_Register
   * @param nSamples number of data points to be recorded in an event packet
   */
  virtual void setEventPacket_NumberOfWaveform(size_t nSamples) {
    vector<uint8_t> writedata;
    writedata.push_back(static_cast<uint8_t>(nSamples << 24 >> 24));
    writedata.push_back(static_cast<uint8_t>(nSamples << 16 >> 24));
    rmapHandler_->write(adcboxRMAPNode, AddressOf_EventPacket_NumberOfWaveform_Register, &writedata[0], 2);
  }

 public:
  // Addresses of Consumer Manager Module
  // clang-format off
  static constexpr uint32_t InitialAddressOf_ConsumerMgr                    = 0x01010000;
  static constexpr uint32_t ConsumerMgrBA                                   = InitialAddressOf_ConsumerMgr;
  static constexpr uint32_t AddressOf_DisableRegister                       = ConsumerMgrBA + 0x0100;
  static constexpr uint32_t AddressOf_AddressUpdateGoRegister               = ConsumerMgrBA + 0x010c;
  static constexpr uint32_t AddressOf_GateSize_FastGate_Register            = ConsumerMgrBA + 0x010e;
  static constexpr uint32_t AddressOf_GateSize_SlowGate_Register            = ConsumerMgrBA + 0x0110;
  static constexpr uint32_t AddressOf_NumberOf_BaselineSample_Register      = ConsumerMgrBA + 0x0112;
  static constexpr uint32_t AddressOf_ConsumerMgr_ResetRegister             = ConsumerMgrBA + 0x0114;
  static constexpr uint32_t AddressOf_EventPacket_NumberOfWaveform_Register = ConsumerMgrBA + 0x0116;
  static constexpr uint32_t AddressOf_Writepointer_Semaphore_Register       = ConsumerMgrBA + 0x0118;
  // Addresses of SDRAM
  static constexpr uint32_t InitialAddressOf_Sdram_EventList = 0x00000000;
  static constexpr uint32_t FinalAddressOf_Sdram_EventList   = 0x00fffffe;
  // clang-format on

 private:
  uint32_t incrementAddress(uint32_t address) {
    if (address == FinalAddressOf_Sdram_EventList) {
      address = InitialAddressOf_Sdram_EventList;
    } else {
      address += 2;
    }
    return address;
  }

  SemaphoreRegister writePointerSemaphore_;

  // Ring buffer
  uint32_t readPointer_{};
  uint32_t writePointer_{};
  uint32_t nextReadFrom_{};
  uint32_t guardBit_{};
};

#endif /* CONSUMERMANAGER_HH_ */
