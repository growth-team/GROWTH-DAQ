#ifndef CHANNELMODULE_HH_
#define CHANNELMODULE_HH_

#include "growth-fpga/registeraccessinterface.hh"
#include "growth-fpga/types.hh"

/// A class which represents a ChannelModule on VHDL logic.
class ChannelModule : public RegisterAccessInterface {
 public:
  static constexpr u32 INITIAL_ADDRESS_CH_MODULE_CH0 = 0x01011000;
  static constexpr u32 ADDRESS_OFFSET_PER_CHANNEL = 0x100;

 public:
  ChannelModule(RMAPInitiatorPtr rmapInitiator, std::shared_ptr<RMAPTargetNode> rmapTargetNode, u8 chNumber);

  /** Sets trigger mode.
   * GROWTH_FY2015_ADC_Type::TriggerMode;<br>
   * StartTh->N_samples = StartThreshold_NSamples_AutoClose<br>
   * Common Gate In = CommonGateIn<br>
   * StartTh->N_samples->ClosingTh = StartThreshold_NSamples_CloseThreshold<br>
   * CPU Trigger = CPUTrigger<br>
   * @param triggerMode trigger mode number
   */
  void setTriggerMode(growth_fpga::TriggerMode triggerMode);

  /** Returns trigger mode.
   * See setTrigger() for returned values.
   */
  u32 getTriggerMode() const;

  void sendCPUTrigger();

  /** Sets number of samples register.
   * Waveforms are sampled according to this number.
   * @param nSamples number of adc samples per one waveform
   */
  void setNumberOfSamples(u16 nSamples);

  /** Sets Leading Trigger Threshold.
   * @param threshold an adc value for leading trigger threshold
   */
  void setStartingThreshold(u16 threshold);

  /** Sets Trailing Trigger Threshold.
   * @param threshold an adc value for trailing trigger threshold
   */
  void setClosingThreshold(u16 threshold);

  /// Turn on/off power of this channle's ADC chip.
  void turnADCPower(bool trueifon);

  /** Sets depth of delay per trigger. When triggered,
   * a waveform will be recorded starting from N steps
   * before of the trigger timing.
   * @param depthOfDelay number of samples retarded
   */
  void setDepthOfDelay(u16 depthOfDelay);

  /// Returns elapsed livetime in 10ms unit.
  u32 getLivetime() const;

  /// Returns current ADC value.
  u16 getCurrentADCValue() const;

  /// Reads TriggerCountRegister.
  size_t getTriggerCount() const;

  /// Reads Status Register. Result will be returned as string.
  std::string getStatus() const;

 private:
  const u8 chNumber_{};
  const u32 BASE_ADDRESS;
  const u32 ADDRESS_TRIGGER_MODE_REGISTER;
  const u32 ADDRESS_NUMBER_OF_SAMPLES_REGISTER;
  const u32 ADDRESS_THRESHOLD_STARTING_REGISTER;
  const u32 ADDRESS_THRESHOLD_CLOSING_REGISTER;
  const u32 ADDRESS_ADC_POWER_DOWN_MODE_REGISTER;
  const u32 ADDRESS_DEPTH_OF_DELAY_REGISTER;
  const u32 ADDRESS_LIVETIME_REGISTER_L;
  const u32 ADDRESS_LIVETIME_REGISTER_H;
  const u32 ADDRESS_CURRENT_ADC_DATA_REGISTER;
  const u32 ADDRESS_CPU_TRIGGER_REGISTER;
  const u32 ADDRESS_STATUS1_REGISTER;
  const u32 ADDRESS_TRIGGER_COUNT_REGISTER_L;
  const u32 ADDRESS_TRIGGER_COUNT_REGISTER_H;
};

#endif /* CHANNELMODULE_HH_ */
