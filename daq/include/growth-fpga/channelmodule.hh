#ifndef CHANNELMODULE_HH_
#define CHANNELMODULE_HH_

#include "growth-fpga/registeraccessinterface.hh"
#include "growth-fpga/types.hh"

/// A class which represents a ChannelModule on VHDL logic.
class ChannelModule : public RegisterAccessInterface {
 public:
  static constexpr u32 InitialAddressOf_ChModule_0 = 0x01011000;
  static constexpr u32 AddressOffsetBetweenChannels = 0x100;

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

  /** Gets Livetime.
   * @return elapsed livetime in 10ms unit
   */
  u32 getLivetime() const;

  /** Get current ADC value.
   * @return temporal ADC value
   */
  u16 getCurrentADCValue() const;

  /** Reads TriggerCountRegister.
   * @return trigger count
   */
  size_t getTriggerCount() const;

  /** Reads Status Register. Result will be returned as string.
   * @return stringified status
   */
  std::string getStatus() const;

 private:
  u8 chNumber_{};

  u32 AddressOf_TriggerModeRegister;
  u32 AddressOf_NumberOfSamplesRegister;
  u32 AddressOf_ThresholdStartingRegister;
  u32 AddressOf_ThresholdClosingRegister;
  u32 AddressOf_AdcPowerDownModeRegister;
  u32 AddressOf_DepthOfDelayRegister;
  u32 AddressOf_LivetimeRegisterL;
  u32 AddressOf_LivetimeRegisterH;
  u32 AddressOf_CurrentAdcDataRegister;
  u32 AddressOf_CPUTriggerRegister;
  u32 AddressOf_Status1Register;
  u32 AddressOf_TriggerCountRegisterL;
  u32 AddressOf_TriggerCountRegisterH;
};

#endif /* CHANNELMODULE_HH_ */
