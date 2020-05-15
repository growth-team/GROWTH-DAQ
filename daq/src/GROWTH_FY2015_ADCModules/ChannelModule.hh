#ifndef CHANNELMODULE_HH_
#define CHANNELMODULE_HH_

#include "GROWTH_FY2015_ADCModules/RegisterAccessInterface.hh"
#include "GROWTH_FY2015_ADCModules/Types.hh"

class RMAPHandler;

/// A class which represents a ChannelModule on VHDL logic.
class ChannelModule : public RegisterAccessInterface {
 public:
  static const uint32_t InitialAddressOf_ChModule_0 = 0x01011000;
  static const uint32_t AddressOffsetBetweenChannels = 0x100;

 public:
  /** Constructor.
   * @param[in] rmaphandler a pointer to an instance of RMAPHandler
   * @param[in] chNumber_ this instance's channel number
   */
  ChannelModule(std::shared_ptr<RMAPHandler> rmapHandler, std::shared_ptr<RMAPTargetNode> rmapTargetNode,
                uint8_t chNumber)
      : RegisterAccessInterface(rmapHandler, rmapTargetNode), chNumber_(chNumber) {
    const uint32_t BA = InitialAddressOf_ChModule_0 + chNumber * AddressOffsetBetweenChannels;
    AddressOf_TriggerModeRegister = BA + 0x0002;
    AddressOf_NumberOfSamplesRegister = BA + 0x0004;
    AddressOf_ThresholdStartingRegister = BA + 0x0006;
    AddressOf_ThresholdClosingRegister = BA + 0x0008;
    AddressOf_AdcPowerDownModeRegister = BA + 0x000a;
    AddressOf_DepthOfDelayRegister = BA + 0x000c;
    AddressOf_LivetimeRegisterL = BA + 0x000e;
    AddressOf_LivetimeRegisterH = BA + 0x0010;
    AddressOf_CurrentAdcDataRegister = BA + 0x0012;
    AddressOf_CPUTriggerRegister = BA + 0x0014;
    AddressOf_TriggerCountRegisterL = BA + 0x0016;
    AddressOf_TriggerCountRegisterH = BA + 0x0018;
    AddressOf_Status1Register = BA + 0x0030;
    AddressOf_TriggerCountRegisterL = AddressOf_TriggerCountRegisterH;
  }

  /** Sets trigger mode.
   * GROWTH_FY2015_ADC_Type::TriggerMode;<br>
   * StartTh->N_samples = StartThreshold_NSamples_AutoClose<br>
   * Common Gate In = CommonGateIn<br>
   * StartTh->N_samples->ClosingTh = StartThreshold_NSamples_CloseThreshold<br>
   * CPU Trigger = CPUTrigger<br>
   * @param triggerMode trigger mode number
   */
  void setTriggerMode(GROWTH_FY2015_ADC_Type::TriggerMode triggerMode) {
    write(AddressOf_TriggerModeRegister, static_cast<uint16_t>(triggerMode));
  }

  /** Returns trigger mode.
   * See setTrigger() for returned values.
   */
  uint32_t getTriggerMode() { return read16(AddressOf_TriggerModeRegister); }

  void sendCPUTrigger() {
    constexpr uint16_t TRIG_FIRE = 0xFFFF;
    write(AddressOf_CPUTriggerRegister, TRIG_FIRE);
  }

  /** Sets number of samples register.
   * Waveforms are sampled according to this number.
   * @param nSamples number of adc samples per one waveform
   */
  void setNumberOfSamples(uint16_t nSamples) { write(AddressOf_NumberOfSamplesRegister, nSamples); }

  /** Sets Leading Trigger Threshold.
   * @param threshold an adc value for leading trigger threshold
   */
  void setStartingThreshold(uint16_t threshold) { write(AddressOf_ThresholdStartingRegister, threshold); }

  /** Sets Trailing Trigger Threshold.
   * @param threshold an adc value for trailing trigger threshold
   */
  void setClosingThreshold(uint16_t threshold) { write(AddressOf_ThresholdClosingRegister, threshold); }

  /// Turn on/off power of this channle's ADC chip.
  void turnADCPower(bool trueifon) {
    constexpr uint16_t POWER_DOWN = 0xFFFF;
    constexpr uint16_t POWER_UP = 0x0000;
    write(AddressOf_AdcPowerDownModeRegister, trueifon ? POWER_UP : POWER_DOWN);
  }

  /** Sets depth of delay per trigger. When triggered,
   * a waveform will be recorded starting from N steps
   * before of the trigger timing.
   * @param depthOfDelay number of samples retarded
   */
  void setDepthOfDelay(uint16_t depthOfDelay) { write(AddressOf_DepthOfDelayRegister, depthOfDelay); }

  /** Gets Livetime.
   * @return elapsed livetime in 10ms unit
   */
  uint32_t getLivetime() const { return read32(AddressOf_LivetimeRegisterL); }

  /** Get current ADC value.
   * @return temporal ADC value
   */
  uint16_t getCurrentADCValue() const { return read16(AddressOf_CurrentAdcDataRegister); }

  /** Reads TriggerCountRegister.
   * @return trigger count
   */
  size_t getTriggerCount() const { return read32(AddressOf_TriggerCountRegisterL); }

  /** Reads Status Register. Result will be returned as string.
   * @return stringified status
   */
  std::string getStatus() const {
    const auto statusRegister = read16(AddressOf_Status1Register);
    std::stringstream ss;
    /*
     Status1Register <= (                             --
     -- Trigger/Veto status
     0      => Veto_internal,                       --1
     1      => InternalModule2ChModule.TriggerOut,  --2
     2      => '0',                                 --4
     3      => ChMgr2ChModule.Veto,                 --8
     4      => TriggerModuleVeto,                   --1
     5      => AdcPowerDownModeRegister(0),         --2
     6      => '1',                                 --4
     7      => '1',                                 --8
     -- EventBuffer status
     8      => BufferNoGood,                        --1
     9      => NoRoomForMoreEvent,                  --2
     10     => hasEvent,                            --4
     others => '1'
     );
     */
    ss << "Veto_internal : " << ((statusRegister & 0x0001) >> 0) << std::endl;
    ss << "TriggerOut    : " << ((statusRegister & 0x0002) >> 1) << std::endl;
    ss << "ChMgr Veto    : " << ((statusRegister & 0x0008) >> 3) << std::endl;
    ss << "TrgModuleVeto : " << ((statusRegister & 0x0010) >> 4) << std::endl;
    ss << "ADCPowerDown  : " << ((statusRegister & 0x0020) >> 5) << std::endl;
    ss << "BufferNoGood  : " << ((statusRegister & 0x0100) >> 8) << std::endl;
    ss << "NoRoomForEvt  : " << ((statusRegister & 0x0200) >> 9) << std::endl;
    ss << "hasEvent      : " << ((statusRegister & 0x0400) >> 10) << std::endl;

    return ss.str();
  }

 private:
  uint8_t chNumber_{};

  uint32_t AddressOf_TriggerModeRegister;
  uint32_t AddressOf_NumberOfSamplesRegister;
  uint32_t AddressOf_ThresholdStartingRegister;
  uint32_t AddressOf_ThresholdClosingRegister;
  uint32_t AddressOf_AdcPowerDownModeRegister;
  uint32_t AddressOf_DepthOfDelayRegister;
  uint32_t AddressOf_LivetimeRegisterL;
  uint32_t AddressOf_LivetimeRegisterH;
  uint32_t AddressOf_CurrentAdcDataRegister;
  uint32_t AddressOf_CPUTriggerRegister;
  uint32_t AddressOf_Status1Register;
  uint32_t AddressOf_TriggerCountRegisterL;
  uint32_t AddressOf_TriggerCountRegisterH;
};

#endif /* CHANNELMODULE_HH_ */
