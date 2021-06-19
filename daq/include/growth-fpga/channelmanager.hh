#ifndef GROWTH_FPGA_CHANNELMANAGER_HH_
#define GROWTH_FPGA_CHANNELMANAGER_HH_

#include "types.h"
#include "growth-fpga/types.hh"
#include "growth-fpga/registeraccessinterface.hh"
#include "growth-fpga/semaphoreregister.hh"

#include <memory>

/** A class which represents ChannelManager module on VHDL logic.
 * This module controls start/stop, preset mode, livetime, and
 * realtime of data acquisition.
 */
class ChannelManager : public RegisterAccessInterface {
 public:
  ChannelManager(std::shared_ptr<RMAPInitiator> rmapInitiator, std::shared_ptr<RMAPTargetNode> rmapTargetNode);
  /// Starts data acquisition.
  /// Configuration of registers of individual channels should be completed before invoking this method.
  /// @param channelsToBeStarted vector of bool, true if the channel should be started
  void startAcquisition(const std::vector<bool>& channelsToBeStarted);

  /// Checks if all data acquisition is completed in all channels.
  bool isAcquisitionCompleted() const;

  /// Checks if data acquisition of single channel is completed.
  bool isAcquisitionCompleted(size_t chNumber) const;

  /** Stops data acquisition regardless of the preset mode of
   * the acquisition.
   */
  void stopAcquisition();

  /** Sets Preset Mode.
   * Available preset modes are:<br>
   * PresetMode::Livetime<br>
   * PresetMode::Number of Event<br>
   * PresetMode::NonStop (Forever)
   * @param presetMode preset mode value (see also enum class PresetMode )
   */
  void setPresetMode(growth_fpga::PresetMode presetMode);

  /** Sets ADC Clock.
   * - ADCClockFrequency::ADCClock200MHz <br>
   * - ADCClockFrequency::ADCClock100MHz <br>
   * - ADCClockFrequency::ADCClock50MHz <br>
   * @param adcClockFrequency enum class ADCClockFrequency
   */
  void setAdcClock(growth_fpga::ADCClockFrequency adcClockFrequency);

  /** Sets Livetime preset value.
   * @param livetimeIn10msUnit live time to be set (in a unit of 10ms)
   */
  void setPresetLivetime(u32 livetimeIn10msUnit);

  /** Get realtime which is elapsed time since the data acquisition was started.
   * @return elapsed real time in 10ms unit
   */
  f64 getRealtime() const;

  /** Resets ChannelManager module on VHDL logic.
   * This method clear all internal registers in the logic module.
   */
  void reset();
  /** Sets PresetnEventsRegister.
   * @param nEvents number of event to be taken
   */
  void setPresetnEvents(u32 nEvents);

  /** Sets ADC clock counter.<br>
   * ADC Clock = 50MHz/(ADC Clock Counter+1)<br>
   * example:<br>
   * ADC Clock 50MHz when ADC Clock Counter=0<br>
   * ADC Clock 25MHz when ADC Clock Counter=1<br>
   * ADC Clock 10MHz when ADC Clock Counter=4
   * @param adcClockCounter ADC Clock Counter value
   */
  void setadcClockCounter(u16 adcClockCounter);

  // Addresses of Channel Manager Module
  // clang-format off
  static constexpr u32 BASE_ADDRESS                          = 0x01010000;  // Base Address of ChMgr
  static constexpr u32 ADDRESS_START_STOP_REGISTER           = BASE_ADDRESS + 0x0002;
  static constexpr u32 ADDRESS_START_STOP_SEMAPHORE_REGISTER = BASE_ADDRESS + 0x0004;
  static constexpr u32 ADDRESS_PRESET_MODE_REGISTER          = BASE_ADDRESS + 0x0006;
  static constexpr u32 ADDRESS_PRESET_LIVETIME_REGISTER_L    = BASE_ADDRESS + 0x0008;
  static constexpr u32 ADDRESS_PRESET_LIVETIME_REGISTER_H    = BASE_ADDRESS + 0x000a;
  static constexpr u32 ADDRESS_REALTIME_REGISTER_L           = BASE_ADDRESS + 0x000c;
  static constexpr u32 ADDRESS_REALTIME_REGISTER_M           = BASE_ADDRESS + 0x000e;
  static constexpr u32 ADDRESS_REALTIME_REGISTER_H           = BASE_ADDRESS + 0x0010;
  static constexpr u32 ADDRESS_RESET_REGISTER                = BASE_ADDRESS + 0x0012;
  static constexpr u32 ADDRESS_ADC_CLOCK_REGISTER            = BASE_ADDRESS + 0x0014;
  static constexpr u32 ADDRESS_PRESETN_EVENTS_REGISTER_L     = BASE_ADDRESS + 0x0020;
  static constexpr u32 ADDRESS_PRESETN_EVENTS_REGISTER_H     = BASE_ADDRESS + 0x0022;
  // clang-format on
  static constexpr f64 LIVETIME_COUNTER_INTERVAL_SEC = 1e-2;  // s (=10ms)

 private:
  SemaphoreRegister startStopSemaphore_;
};

#endif /* CHANNELMANAGER_HH_ */
