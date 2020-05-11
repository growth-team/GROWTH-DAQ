#ifndef CHANNELMANAGER_HH_
#define CHANNELMANAGER_HH_

#include "GROWTH_FY2015_ADCModules/RMAPHandler.hh"
#include "GROWTH_FY2015_ADCModules/RegisterAccessInterface.hh"
#include "GROWTH_FY2015_ADCModules/SemaphoreRegister.hh"
#include "GROWTH_FY2015_ADCModules/Types.hh"

#include <array>
#include <bitset>
#include <memory>

/** A class which represents ChannelManager module on VHDL logic.
 * This module controls start/stop, preset mode, livetime, and
 * realtime of data acquisition.
 */
class ChannelManager : public RegisterAccessInterface {
 public:
  ChannelManager(std::shared_ptr<RMAPHandler> rmapHandler)
      : RegisterAccessInterface(rmapHandler), startStopSemaphore_(rmapHandler, AddressOf_StartStopSemaphoreRegister) {}

  /// Starts data acquisition.
  /// Configuration of registers of individual channels should be completed before invoking this method.
  /// @param channelsToBeStarted vector of bool, true if the channel should be started
  void startAcquisition(const std::vector<bool>& channelsToBeStarted) {
    // Prepare register value for StartStopRegister
    const uint16_t value = [&]() {
      uint16_t value = 0;
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
  bool isAcquisitionCompleted() {
    const uint16_t value = read16(AddressOf_StartStopRegister);
    if (value == 0x0000) {
      return true;
    } else {
      return false;
    }
  }

  /// Checks if data acquisition of single channel is completed.
  bool isAcquisitionCompleted(size_t chNumber) {
    const uint16_t value = read16(AddressOf_StartStopRegister);
    if (chNumber < GROWTH_FY2015_ADC_Type::NumberOfChannels) {
      const std::bitset<16> bits(value);
      if (bits[chNumber] == 0) {
        return true;
      } else {
        return false;
      }
    } else {
      // if chNumber is out of the allowed range
      return false;
    }
  }

  /** Stops data acquisition regardless of the preset mode of
   * the acquisition.
   */
  void stopAcquisition() {
    constexpr uint16_t STOP = 0x00;
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
  void setPresetMode(GROWTH_FY2015_ADC_Type::PresetMode presetMode) {
    write(AddressOf_PresetModeRegister, static_cast<uint16_t>(presetMode));
  }

  /** Sets ADC Clock.
   * - ADCClockFrequency::ADCClock200MHz <br>
   * - ADCClockFrequency::ADCClock100MHz <br>
   * - ADCClockFrequency::ADCClock50MHz <br>
   * @param adcClockFrequency enum class ADCClockFrequency
   */
  void setAdcClock(GROWTH_FY2015_ADC_Type::ADCClockFrequency adcClockFrequency) {
    write(AddressOf_ADCClock_Register, static_cast<uint16_t>(adcClockFrequency));
  }

  /** Sets Livetime preset value.
   * @param livetimeIn10msUnit live time to be set (in a unit of 10ms)
   */
  void setPresetLivetime(uint32_t livetimeIn10msUnit) { write(AddressOf_PresetLivetimeRegisterL, livetimeIn10msUnit); }

  /** Get Realtime which is elapsed time since the data acquisition was started.
   * @return elapsed real time in 10ms unit
   */
  double getRealtime() { return static_cast<double>(read48(AddressOf_RealtimeRegisterL)); }

  /** Resets ChannelManager module on VHDL logic.
   * This method clear all internal registers in the logic module.
   */
  void reset() {
    constexpr uint16_t RESET = 0x01;
    write(AddressOf_ResetRegister, RESET);
  }

  /** Sets PresetnEventsRegister.
   * @param nEvents number of event to be taken
   */
  void setPresetnEvents(uint32_t nEvents) { write(AddressOf_PresetLivetimeRegisterL, nEvents); }

  /** Sets ADC clock counter.<br>
   * ADC Clock = 50MHz/(ADC Clock Counter+1)<br>
   * example:<br>
   * ADC Clock 50MHz when ADC Clock Counter=0<br>
   * ADC Clock 25MHz when ADC Clock Counter=1<br>
   * ADC Clock 10MHz when ADC Clock Counter=4
   * @param adcClockCounter ADC Clock Counter value
   */
  void setadcClockCounter(uint16_t adcClockCounter) { write(AddressOf_ADCClock_Register, adcClockCounter); }

  // Addresses of Channel Manager Module
  // clang-format off
  static constexpr uint32_t InitialAddressOf_ChMgr = 0x01010000;
  static constexpr uint32_t ChMgrBA                = InitialAddressOf_ChMgr;  // Base Address of ChMgr
  static constexpr uint32_t AddressOf_StartStopRegister          = ChMgrBA + 0x0002;
  static constexpr uint32_t AddressOf_StartStopSemaphoreRegister = ChMgrBA + 0x0004;
  static constexpr uint32_t AddressOf_PresetModeRegister         = ChMgrBA + 0x0006;
  static constexpr uint32_t AddressOf_PresetLivetimeRegisterL    = ChMgrBA + 0x0008;
  static constexpr uint32_t AddressOf_PresetLivetimeRegisterH    = ChMgrBA + 0x000a;
  static constexpr uint32_t AddressOf_RealtimeRegisterL          = ChMgrBA + 0x000c;
  static constexpr uint32_t AddressOf_RealtimeRegisterM          = ChMgrBA + 0x000e;
  static constexpr uint32_t AddressOf_RealtimeRegisterH          = ChMgrBA + 0x0010;
  static constexpr uint32_t AddressOf_ResetRegister              = ChMgrBA + 0x0012;
  static constexpr uint32_t AddressOf_ADCClock_Register          = ChMgrBA + 0x0014;
  static constexpr uint32_t AddressOf_PresetnEventsRegisterL     = ChMgrBA + 0x0020;
  static constexpr uint32_t AddressOf_PresetnEventsRegisterH     = ChMgrBA + 0x0022;
  // clang-format on
  static constexpr double LivetimeCounterInterval = 1e-2;  // s (=10ms)

 private:
  SemaphoreRegister startStopSemaphore_;
};

#endif /* CHANNELMANAGER_HH_ */
