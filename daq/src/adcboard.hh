#ifndef GROWTHDAQ_ADCBOARD_HH_
#define GROWTHDAQ_ADCBOARD_HH_

#include "growth-fpga/types.hh"
#include "types.h"
#include "yaml-cpp/yaml.h"

#include <array>
#include <memory>
#include <string>
#include <vector>

class ChannelManager;
class ChannelModule;
class ConsumerManagerEventFIFO;
class EventDecoder;
class RMAPHandlerUART;
class RMAPInitiator;
class RMAPTargetNode;
class RegisterAccessInterface;

using growth_fpga::TriggerMode;

enum class SpaceFibreADCException {
  InvalidChannelNumber,
  OpenDeviceFailed,
  CloseDeviceFailed,
};

/** A class which represents SpaceFibre ADC Board.
 * It controls start/stop of data acquisition.
 * It contains GROWTH_FY2015_ADC::SpaceFibreADC::NumberOfChannels instances of
 * ADCChannel class so that
 * a user can change individual registers in each module.
 */
class GROWTH_FY2015_ADC {
 public:
  // Clock Frequency
  static constexpr f64 ClockFrequency = 100;  // MHz
  // Clock Interval
  static constexpr f64 ClockInterval = 10e-9;  // s
  // FPGA TimeTag
  static constexpr u32 TimeTagResolutionInNanoSec = 10;  // ns
  // PHA Min/Max
  static constexpr u16 PHAMinimum = 0;
  static constexpr u16 PHAMaximum = 4095;
  static constexpr size_t GPS_TIME_REG_SIZE_BYTES = 20;

 public:
  /** Constructor.
   * @param deviceName UART-USB device name (e.g. /dev/tty.usb-aaa-bbb)
   */
  GROWTH_FY2015_ADC(std::string deviceName);
  ~GROWTH_FY2015_ADC();

  u32 getFPGAType() const;
  u32 getFPGAVersion() const;
  std::string getGPSRegister() const;
  std::array<u8, GROWTH_FY2015_ADC::GPS_TIME_REG_SIZE_BYTES + 1> getGPSRegisterUInt8() const;

  /** Clears the GPS Data FIFO.
   *  After clear, new data coming from GPS Receiver will be written to GPS Data FIFO.
   */
  void clearGPSDataFIFO();

  /** Reads GPS Data FIFO.
   *  After clear, new data coming from GPS Receiver will be written to GPS Data FIFO.
   */
  std::vector<u8> readGPSDataFIFO();

  /** Reset ChannelManager and ConsumerManager modules on VHDL.
   */
  void reset();

  /** Reads, decodes, and returns event data recorded by the board.
   * When no event packet is received within a timeout duration,
   * this method will return empty vector meaning a time out.
   * Each GROWTH_FY2015_ADC_Type::Event instances pointed by the entities
   * of the returned vector should be freed after use in the user
   * application. A freed GROWTH_FY2015_ADC_Type::Event instance will be
   * reused to represent another event data.
   * @return a vector containing pointers to decoded event data
   */
  std::vector<growth_fpga::Event*> getEvent();

  /** Frees an event instance so that buffer area can be reused in the following commands.
   * @param[in] event event instance to be freed
   */
  void freeEvent(growth_fpga::Event* event);

  /** Frees event instances so that buffer area can be reused in the following commands.
   * @param[in] events a vector of event instance to be freed
   */
  void freeEvents(std::vector<growth_fpga::Event*>& events);

  /** Set trigger mode of the specified channel.
   * TriggerMode;<br>
   * StartTh->N_samples = StartThreshold_NSamples_AutoClose<br>
   * Common Gate In = CommonGateIn<br>
   * StartTh->N_samples->ClosingTh = StartThreshold_NSamples_CloseThreshold<br>
   * CPU Trigger = CPUTrigger<br>
   * Trigger Bus (OR) = TriggerBusSelectedOR<br>
   * Trigger Bus (AND) = TriggerBusSelectedAND<br>
   * @param chNumber channel number
   * @param triggerMode trigger mode (see TriggerMode)
   */
  void setTriggerMode(size_t chNumber, growth_fpga::TriggerMode triggerMode);

  /** Sets the number of ADC samples recorded for a single trigger.
   * This method sets the same number to all the channels.
   * After the trigger, nSamples set via this method will be recorded
   * in the waveform buffer,
   * and then analyzed in the Pulse Processing Module to form an event packet.
   * In this processing, the waveform can be truncated at a specified length,
   * and see setNumberOfSamplesInEventPacket(u16 nSamples) for this
   * truncating length setting.
   * @note As of 20141103, the maximum waveform length is 1000.
   * This value can be increased by modifying the depth of the event FIFO
   * in the FPGA.
   * @param nSamples number of adc samples per one waveform data
   */
  void setNumberOfSamples(u16 nSamples);

  /** Sets the number of ADC samples which should be output in the
   * event packet (this must be smaller than the number of samples
   * recorded for a trigger, see setNumberOfSamples(u16 nSamples) ).
   * @param[in] nSamples number of ADC samples in the event packet
   */
  void setNumberOfSamplesInEventPacket(u16 nSamples);

  /** Sets Leading Trigger Threshold.
   * @param threshold an adc value for leading trigger threshold
   */
  void setStartingThreshold(size_t chNumber, u32 threshold);

  /** Sets Trailing Trigger Threshold.
   * @param threshold an adc value for trailing trigger threshold
   */
  void setClosingThreshold(size_t chNumber, u32 threshold);

  /** Sets depth of delay per trigger. When triggered,
   * a waveform will be recorded starting from N steps
   * before of the trigger timing.
   * @param depthOfDelay number of samples retarded
   */
  void setDepthOfDelay(size_t chNumber, u32 depthOfDelay);

  /** Gets Livetime.
   * @return elapsed livetime in 10ms unit
   */
  u32 getLivetime(size_t chNumber) const;
  /** Get current ADC value.
   * @return temporal ADC value
   */
  u32 getCurrentADCValue(size_t chNumber) const;

  /** Turn on ADC.
   */
  void turnOnADCPower(size_t chNumber);

  /** Turn off ADC.
   */
  void turnOffADCPower(size_t chNumber);

  /** Starts data acquisition. Configuration of registers of
   * individual channels should be completed before invoking this method.
   * @param channelsToBeStarted vector of bool, true if the channel should be started
   */
  void startAcquisition(std::vector<bool> channelsToBeStarted);

  /** Starts data acquisition. Configuration of registers of
   * individual channels should be completed before invoking this method.
   * Started channels should have been provided via loadConfigurationFile().
   * @param channelsToBeStarted vector of bool, true if the channel should be started
   */
  void startAcquisition();

  /** Checks if all data acquisition is completed in all channels.
   * 	return true if data acquisition is stopped.
   */
  bool isAcquisitionCompleted() const;

  /** Checks if data acquisition of single channel is completed.
   * 	return true if data acquisition is stopped.
   */
  bool isAcquisitionCompleted(size_t chNumber) const;

  /** Stops data acquisition regardless of the preset mode of the acquisition.
   */
  void stopAcquisition();

  /** Sends CPU Trigger to force triggering in the specified channel.
   * @param[in] chNumber channel to be CPU-triggered
   */
  void sendCPUTrigger(size_t chNumber);

  /** Sends CPU Trigger to all enabled channels.
   * @param[in] chNumber channel to be CPU-triggered
   */
  void sendCPUTrigger();

  /** Sets measurement preset mode.
   * Available preset modes are:<br>
   * PresetMode::Livetime<br>
   * PresetMode::Number of Event<br>
   * PresetMode::NonStop (Forever)
   * @param presetMode preset mode value (see also enum class SpaceFibreADC::PresetMode )
   */
  void setPresetMode(growth_fpga::PresetMode presetMode);

  /** Sets Livetime preset value.
   * @param livetimeIn10msUnit live time to be set (in a unit of 10ms)
   */
  void setPresetLivetime(u32 livetimeIn10msUnit);

  /** Sets PresetnEventsRegister.
   * @param nEvents number of event to be taken
   */
  void setPresetnEvents(u32 nEvents);

  /** Get real time which is elapsed time since the board power was turned on.
   * @return elapsed real time in 10ms unit
   */
  f64 getRealtime() const;

  /** Sets ADC Clock.
   * - ADCClockFrequency::ADCClock200MHz <br>
   * - ADCClockFrequency::ADCClock100MHz <br>
   * - ADCClockFrequency::ADCClock50MHz <br>
   * @param adcClockFrequency enum class ADCClockFrequency
   */
  void setAdcClock(growth_fpga::ADCClockFrequency adcClockFrequency);

  /** Gets HK data including the real time and the live time which are
   * counted in the FPGA, and acquisition status (started or stopped).
   * @retrun HK information contained in a SpaceFibreADC::HouseKeepingData instance
   */
  growth_fpga::HouseKeepingData getHouseKeepingData() const;

  size_t getNSamplesInEventListFile() const;
  void dumpMustExistKeywords();
  void loadConfigurationFile(const std::string& inputFileName);

  const size_t nChannels = 4;
  std::string DetectorID{};
  size_t PreTriggerSamples = 4;
  size_t PostTriggerSamples = 1000;
  std::vector<enum TriggerMode> TriggerModes{
      TriggerMode::StartThreshold_NSamples_CloseThreshold, TriggerMode::StartThreshold_NSamples_CloseThreshold,
      TriggerMode::StartThreshold_NSamples_CloseThreshold, TriggerMode::StartThreshold_NSamples_CloseThreshold};
  size_t SamplesInEventPacket = 1000;
  size_t DownSamplingFactorForSavedWaveform = 1;
  std::vector<bool> ChannelEnable{};
  std::vector<u16> TriggerThresholds{};
  std::vector<u16> TriggerCloseThresholds{};

 private:
  template <typename T, typename Y>
  std::map<T, Y> parseYAMLMap(YAML::Node node) {
    std::map<T, Y> outputMap;
    for (auto e : node) {
      outputMap.insert({e.first.as<T>(), e.second.as<Y>()});
    }
    return outputMap;
  }

  void dumpThread();

  // clang-format off
  static constexpr u32 AddressOfGPSTimeRegister          = 0x20000002;
  static constexpr u32 AddressOfGPSDataFIFOResetRegister = 0x20001000;
  static constexpr u32 InitialAddressOfGPSDataFIFO       = 0x20001002;
  static constexpr u32 FinalAddressOfGPSDataFIFO         = 0x20001FFF;

  static constexpr u32 AddressOfFPGATypeRegister_L    = 0x30000000;
  static constexpr u32 AddressOfFPGATypeRegister_H    = 0x30000002;
  static constexpr u32 AddressOfFPGAVersionRegister_L = 0x30000004;
  static constexpr u32 AddressOfFPGAVersionRegister_H = 0x30000006;
  // clang-format on

  std::shared_ptr<RMAPHandlerUART> rmapHandler_;
  std::shared_ptr<RMAPTargetNode> adcRMAPTargetNode_;
  std::unique_ptr<RMAPInitiator> rmapIniaitorForGPSRegisterAccess_;
  std::unique_ptr<EventDecoder> eventDecoder_;
  std::unique_ptr<ChannelManager> channelManager_;
  std::unique_ptr<ConsumerManagerEventFIFO> consumerManager_;
  using ChannelModulePtr = std::unique_ptr<ChannelModule>;
  std::vector<ChannelModulePtr> channelModules_;

  std::shared_ptr<RegisterAccessInterface> reg_;

  size_t nReceivedEvents_ = 0;
  const size_t GPS_DATA_FIFO_DEPTH_BYTES = 1024;
  std::unique_ptr<u8[]> gpsDataFIFOReadBuffer_;
  std::vector<u8> gpsDataFIFOData_{};
  std::vector<growth_fpga::Event*> events_{};
  bool stopDumpThread_ = false;
};

#endif /* ADCBOARD_HH_ */
