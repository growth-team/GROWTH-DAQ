#ifndef GROWTH_FPGA_CHANNELMANAGER_HH_
#define GROWTH_FPGA_CHANNELMANAGER_HH_

#include "types.h"
#include "growth-fpga/types.hh"
#include "growth-fpga/registeraccessinterface.hh"
#include "growth-fpga/rmaphandleruart.hh"
#include "growth-fpga/semaphoreregister.hh"

#include <array>
#include <bitset>
#include <memory>


/** A class which represents ChannelManager module on VHDL logic.
 * This module controls start/stop, preset mode, livetime, and
 * realtime of data acquisition.
 */
class ChannelManager : public RegisterAccessInterface {
 public:
  ChannelManager(std::shared_ptr<RMAPHandlerUART> rmapHandler, std::shared_ptr<RMAPTargetNode> rmapTargetNode)
      : RegisterAccessInterface(rmapHandler, rmapTargetNode),
        startStopSemaphore_(rmapHandler, rmapTargetNode, AddressOf_StartStopSemaphoreRegister) {}

  /// Starts data acquisition.
  /// Configuration of registers of individual channels should be completed before invoking this method.
  /// @param channelsToBeStarted vector of bool, true if the channel should be started
  void startAcquisition(const std::vector<bool>& channelsToBeStarted) {
    // Prepare register value for StartStopRegister
    const u16 value = [&]() {
      u16 value = 0;
      for (const auto bit : channelsToBeStarted) {
        value = (value << 1) | (bit ? 1 : 0);
      }
      return value;
    }();
    // write the StartStopRegister (semaphore is needed)
    SemaphoreLock lock(startStopSemaphore_);
    write(AddressOf_StartStopRegister, value);
  }

  /// Checks if all data acquisition is completed in all channels.
  bool isAcquisitionCompleted() const {
    const u16 value = read16(AddressOf_StartStopRegister);
    return value == 0x0000;
  }

  /// Checks if data acquisition of single channel is completed.
  bool isAcquisitionCompleted(size_t chNumber) const {
    const u16 value = read16(AddressOf_StartStopRegister);
    if (chNumber < growth_fpga::NumberOfChannels) {
      const std::bitset<16> bits(value);
      return bits[chNumber] == 0;
    } else {
      // if chNumber is out of the allowed range
      return false;
    }
  }

  /** Stops data acquisition regardless of the preset mode of
   * the acquisition.
   */
  void stopAcquisition() {
    constexpr u16 STOP = 0x00;
    SemaphoreLock lock(startStopSemaphore_);
    write(AddressOf_StartStopRegister, STOP);
  }

  /** Sets Preset Mode.
   * Available preset modes are:<br>
   * PresetMode::Livetime<br>
   * PresetMode::Number of Event<br>
   * PresetMode::NonStop (Forever)
   * @param presetMode preset mode value (see also enum class PresetMode )
   */
  void setPresetMode(growth_fpga::PresetMode presetMode) {
    write(AddressOf_PresetModeRegister, static_cast<u16>(presetMode));
  }

  /** Sets ADC Clock.
   * - ADCClockFrequency::ADCClock200MHz <br>
   * - ADCClockFrequency::ADCClock100MHz <br>
   * - ADCClockFrequency::ADCClock50MHz <br>
   * @param adcClockFrequency enum class ADCClockFrequency
   */
  void setAdcClock(growth_fpga::ADCClockFrequency adcClockFrequency) {
    write(AddressOf_ADCClock_Register, static_cast<u16>(adcClockFrequency));
  }

  /** Sets Livetime preset value.
   * @param livetimeIn10msUnit live time to be set (in a unit of 10ms)
   */
  void setPresetLivetime(u32 livetimeIn10msUnit) { write(AddressOf_PresetLivetimeRegisterL, livetimeIn10msUnit); }

  /** Get Realtime which is elapsed time since the data acquisition was started.
   * @return elapsed real time in 10ms unit
   */
  f64 getRealtime() const { return static_cast<f64>(read48(AddressOf_RealtimeRegisterL)); }

  /** Resets ChannelManager module on VHDL logic.
   * This method clear all internal registers in the logic module.
   */
  void reset() {
    constexpr u16 RESET = 0x01;
    write(AddressOf_ResetRegister, RESET);
  }

  /** Sets PresetnEventsRegister.
   * @param nEvents number of event to be taken
   */
  void setPresetnEvents(u32 nEvents) { write(AddressOf_PresetLivetimeRegisterL, nEvents); }

  /** Sets ADC clock counter.<br>
   * ADC Clock = 50MHz/(ADC Clock Counter+1)<br>
   * example:<br>
   * ADC Clock 50MHz when ADC Clock Counter=0<br>
   * ADC Clock 25MHz when ADC Clock Counter=1<br>
   * ADC Clock 10MHz when ADC Clock Counter=4
   * @param adcClockCounter ADC Clock Counter value
   */
  void setadcClockCounter(u16 adcClockCounter) { write(AddressOf_ADCClock_Register, adcClockCounter); }

  // Addresses of Channel Manager Module
  // clang-format off
  static constexpr u32 InitialAddressOf_ChMgr = 0x01010000;
  static constexpr u32 ChMgrBA                = InitialAddressOf_ChMgr;  // Base Address of ChMgr
  static constexpr u32 AddressOf_StartStopRegister          = ChMgrBA + 0x0002;
  static constexpr u32 AddressOf_StartStopSemaphoreRegister = ChMgrBA + 0x0004;
  static constexpr u32 AddressOf_PresetModeRegister         = ChMgrBA + 0x0006;
  static constexpr u32 AddressOf_PresetLivetimeRegisterL    = ChMgrBA + 0x0008;
  static constexpr u32 AddressOf_PresetLivetimeRegisterH    = ChMgrBA + 0x000a;
  static constexpr u32 AddressOf_RealtimeRegisterL          = ChMgrBA + 0x000c;
  static constexpr u32 AddressOf_RealtimeRegisterM          = ChMgrBA + 0x000e;
  static constexpr u32 AddressOf_RealtimeRegisterH          = ChMgrBA + 0x0010;
  static constexpr u32 AddressOf_ResetRegister              = ChMgrBA + 0x0012;
  static constexpr u32 AddressOf_ADCClock_Register          = ChMgrBA + 0x0014;
  static constexpr u32 AddressOf_PresetnEventsRegisterL     = ChMgrBA + 0x0020;
  static constexpr u32 AddressOf_PresetnEventsRegisterH     = ChMgrBA + 0x0022;
  // clang-format on
  static constexpr f64 LivetimeCounterInterval = 1e-2;  // s (=10ms)

 private:
  SemaphoreRegister startStopSemaphore_;
};

#endif /* CHANNELMANAGER_HH_ */