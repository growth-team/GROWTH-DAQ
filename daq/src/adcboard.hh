#ifndef ADCBOARD_HH_
#define ADCBOARD_HH_

/** @mainpage GROWTH ADC C++ API
 * # Overview
 * GROWTH FY2015 ADC board carries 4-ch 65-Msps pipeline ADC,
 * USB-UART interface, FPGA, and Raspberry Pi.
 * adcboard.hh and related header files provide API for
 * communicating with the GROWTH ADC board using C++.
 *
 * # Structure
 * ADCBoard is the top-level class which provides
 * all user-level methods to control and configure the board,
 * to set/start/stop the evnet acquisition mode, and
 * to decode recorded event data and waveform.
 * For available methods, see adcboard page.
 *
 * # Typical usage
 * -# Include the following header files.
 *   - SpaceWireRMAPLibrary/SpaceWire.hh
 *   - SpaceWireRMAPLibrary/RMAP.hh
 *   - SpaceWireRMAPLibrary/Boards/SpaceFibreADC.hh
 * -# Instantiate the GROWTH_FY2015_ADC class.
 * -# Do configuration:
 *   - Trigger mode.
 *   - Threshold.
 *   - Waveform record length.
 *   - Preset mode (to automatically stop measurement).
 *     - Livetime preset.
 *     - Event count preset.
 *     - No preset (measurement continues forever unless manually stopped).
 * -# Start measurement.
 * -# Read event data. Loop until measurement completes.
 * -# Stop measurement.
 * -# Close device.
 *
 * # Contact
 * - Post issues on github.com/growth-team/growth-daq
 *
 * # Example user application
 * @code
 //---------------------------------------------
 // Include SpaceWireRMAPLibrary
 #include "SpaceWireRMAPLibrary/RMAP.hh"
 #include "SpaceWireRMAPLibrary/SpaceWire.hh"

 //---------------------------------------------
 // Include SpaceFibreADC
 #include "SpaceWireRMAPLibrary/Boards/SpaceFibreADC.hh"

 const size_t nSamples = 200;
 const size_t nCPUTrigger=10;

 int main(int argc, char* argv[]){
 using namespace std;

 //---------------------------------------------
 // Construct an instance of GROWTH_FY2015_ADC
 // The default IP address is 192.168.1.100 .
 // If your board has set a specific IP address,
 // modify the following line.
 GROWTH_FY2015_ADC* adc=new GROWTH_FY2015_ADC("192.168.1.100");

 //---------------------------------------------
 // Device open
 cout << "Opening device" << endl;
 adc->openDevice();
 cout << "Opened" << endl;

 //---------------------------------------------
 // Trigger Mode
 size_t ch=0;
 cout << "Setting trigger mode" << endl;

 // Trigger Mode: CPU Trigger
 adc->setTriggerMode(ch, TriggerMode::CPUTrigger);

 // Trigger Mode: StartingThreshold-NSamples-ClosingThreshold
 // uint16_t startingThreshold=2300;
 // uint16_t closingThreshold=2100;
 // adc->setStartingThreshold(ch, startingThreshold);
 // adc->setClosingThreshold(ch, closingThreshold);
 //adc->setTriggerMode(ch, TriggerMode::StartThreshold_NSamples_CloseThreshold);

 //---------------------------------------------
 // Waveform record length
 cout << "Setting number of samples" << endl;
 adc->setNumberOfSamples(200);
 cout << "Setting depth of delay" << endl;
 adc->setDepthOfDelay(ch, 20);
 cout << "Setting ADC power" << endl;
 adc->turnOnADCPower(ch);


 //---------------------------------------------
 // Start data acquisition
 cout << "Starting acquisition" << endl;
 adc->startAcquisition({true,false,false,false});


 //---------------------------------------------
 // Send CPU Trigger
 cout << "Sending CPU Trigger " << dec << nCPUTrigger << " times" << endl;
 for(size_t i=0;i<nCPUTrigger;i++){
 adc->sendCPUTrigger(ch);
 }


 //---------------------------------------------
 // Read event data
 std::vector<GROWTH_FY2015_ADC_Type::Event*> events;

 cout << "Reading events" << endl;
 events=adc->getEvent();
 cout << "nEvents = " << events.size() << endl;

 size_t i=0;
 for(auto event : events){
 cout << dec;
 cout << "=============================================" << endl;
 cout << "Event " << i << endl;
 cout << "---------------------------------------------" << endl;
 cout << "timeTag = " << event->timeTag << endl;
 cout << "triggerCount = " << event->triggerCount << endl;
 cout << "phaMax = " << (uint32_t)event->phaMax << endl;
 cout << "nSamples = " << (uint32_t)event->nSamples << endl;
 cout << "waveform = ";
 for(size_t i=0;i<event->nSamples;i++){
 cout << dec << (uint32_t)event->waveform[i] << " ";
 }
 cout << endl;
 i++;
 adc->freeEvent(event);
 }

 //---------------------------------------------
 // Reads HK data
 SpaceFibreADC::HouseKeepingData hk=adc->getHouseKeepingData();
 cout << "livetime = " << hk.livetime[0] << endl;
 cout << "realtime = " << hk.realtime << endl;
 cout << dec;

 //---------------------------------------------
 // Reads current ADC value (un-comment out the following lines to execute)
 // cout << "Current ADC values:"  << endl;
 // for(size_t i=0;i<10;i++){
 // 	cout << adc->getCurrentADCValue(ch) << endl;
 // }

 //---------------------------------------------
 // Stop data acquisition
 cout << "Stopping acquisition" << endl;
 adc->stopAcquisition();

 //---------------------------------------------
 // Device close
 cout << "Closing device" << endl;
 adc->closeDevice();
 cout << "Closed" << endl;

 }
 * @endcode
 */

#include "GROWTH_FY2015_ADCModules/Types.hh"
#include "yaml-cpp/yaml.h"

#include <memory>
#include <string>
#include <vector>

class ChannelManager;
class ChannelModule;
class ConsumerManagerEventFIFO;
class EventDecoder;
class RMAPHandlerUART;
class RMAPInitiator;

using GROWTH_FY2015_ADC_Type::TriggerMode;

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
  /** Constructor.
   * @param deviceName UART-USB device name (e.g. /dev/tty.usb-aaa-bbb)
   */
  GROWTH_FY2015_ADC(std::string deviceName);
  ~GROWTH_FY2015_ADC();

  uint32_t getFPGAType();
  uint32_t getFPGAVersion();
  std::string getGPSRegister();
  uint8_t* getGPSRegisterUInt8();

  /** Clears the GPS Data FIFO.
   *  After clear, new data coming from GPS Receiver will be written to GPS Data FIFO.
   */
  void clearGPSDataFIFO();

  /** Reads GPS Data FIFO.
   *  After clear, new data coming from GPS Receiver will be written to GPS Data FIFO.
   */
  std::vector<uint8_t> readGPSDataFIFO();

  /** Reset ChannelManager and ConsumerManager modules on VHDL.
   */
  void reset();

  /** Closes the device.
   */
  void closeDevice();

  /** Reads, decodes, and returns event data recorded by the board.
   * When no event packet is received within a timeout duration,
   * this method will return empty vector meaning a time out.
   * Each GROWTH_FY2015_ADC_Type::Event instances pointed by the entities
   * of the returned vector should be freed after use in the user
   * application. A freed GROWTH_FY2015_ADC_Type::Event instance will be
   * reused to represent another event data.
   * @return a vector containing pointers to decoded event data
   */
  std::vector<GROWTH_FY2015_ADC_Type::Event*> getEvent();

  /** Frees an event instance so that buffer area can be reused in the following commands.
   * @param[in] event event instance to be freed
   */
  void freeEvent(GROWTH_FY2015_ADC_Type::Event* event);

  /** Frees event instances so that buffer area can be reused in the following commands.
   * @param[in] events a vector of event instance to be freed
   */
  void freeEvents(std::vector<GROWTH_FY2015_ADC_Type::Event*>& events);

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
  void setTriggerMode(size_t chNumber, TriggerMode triggerMode);

  /** Sets the number of ADC samples recorded for a single trigger.
   * This method sets the same number to all the channels.
   * After the trigger, nSamples set via this method will be recorded
   * in the waveform buffer,
   * and then analyzed in the Pulse Processing Module to form an event packet.
   * In this processing, the waveform can be truncated at a specified length,
   * and see setNumberOfSamplesInEventPacket(uint16_t nSamples) for this
   * truncating length setting.
   * @note As of 20141103, the maximum waveform length is 1000.
   * This value can be increased by modifying the depth of the event FIFO
   * in the FPGA.
   * @param nSamples number of adc samples per one waveform data
   */
  void setNumberOfSamples(uint16_t nSamples);

  /** Sets the number of ADC samples which should be output in the
   * event packet (this must be smaller than the number of samples
   * recorded for a trigger, see setNumberOfSamples(uint16_t nSamples) ).
   * @param[in] nSamples number of ADC samples in the event packet
   */
  void setNumberOfSamplesInEventPacket(uint16_t nSamples);

  /** Sets Leading Trigger Threshold.
   * @param threshold an adc value for leading trigger threshold
   */
  void setStartingThreshold(size_t chNumber, uint32_t threshold);

  /** Sets Trailing Trigger Threshold.
   * @param threshold an adc value for trailing trigger threshold
   */
  void setClosingThreshold(size_t chNumber, uint32_t threshold);

  /** Sets depth of delay per trigger. When triggered,
   * a waveform will be recorded starting from N steps
   * before of the trigger timing.
   * @param depthOfDelay number of samples retarded
   */
  void setDepthOfDelay(size_t chNumber, uint32_t depthOfDelay);

  /** Gets Livetime.
   * @return elapsed livetime in 10ms unit
   */
  uint32_t getLivetime(size_t chNumber);
  /** Get current ADC value.
   * @return temporal ADC value
   */
  uint32_t getCurrentADCValue(size_t chNumber);

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
  bool isAcquisitionCompleted();

  /** Checks if data acquisition of single channel is completed.
   * 	return true if data acquisition is stopped.
   */
  bool isAcquisitionCompleted(size_t chNumber);

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
  void setPresetMode(GROWTH_FY2015_ADC_Type::PresetMode presetMode);

  /** Sets Livetime preset value.
   * @param livetimeIn10msUnit live time to be set (in a unit of 10ms)
   */
  void setPresetLivetime(uint32_t livetimeIn10msUnit);

  /** Sets PresetnEventsRegister.
   * @param nEvents number of event to be taken
   */
  void setPresetnEvents(uint32_t nEvents);

  /** Get Realtime which is elapsed time since the board power was turned on.
   * @return elapsed real time in 10ms unit
   */
  double getRealtime();

  /** Sets ADC Clock.
   * - ADCClockFrequency::ADCClock200MHz <br>
   * - ADCClockFrequency::ADCClock100MHz <br>
   * - ADCClockFrequency::ADCClock50MHz <br>
   * @param adcClockFrequency enum class ADCClockFrequency
   */
  void setAdcClock(GROWTH_FY2015_ADC_Type::ADCClockFrequency adcClockFrequency);

  /** Gets HK data including the real time and the live time which are
   * counted in the FPGA, and acquisition status (started or stopped).
   * @retrun HK information contained in a SpaceFibreADC::HouseKeepingData instance
   */
  GROWTH_FY2015_ADC_Type::HouseKeepingData getHouseKeepingData();

  size_t getNSamplesInEventListFile();
  void dumpMustExistKeywords();
  void loadConfigurationFile(std::string inputFileName);

 public:
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
  std::vector<uint16_t> TriggerThresholds{};
  std::vector<uint16_t> TriggerCloseThresholds{};

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
  static constexpr uint32_t AddressOfGPSTimeRegister          = 0x20000002;
  static constexpr size_t   LengthOfGPSTimeRegister           = 20;
  static constexpr uint32_t AddressOfGPSDataFIFOResetRegister = 0x20001000;
  static constexpr uint32_t InitialAddressOfGPSDataFIFO       = 0x20001002;
  static constexpr uint32_t FinalAddressOfGPSDataFIFO         = 0x20001FFF;

  static constexpr uint32_t AddressOfFPGATypeRegister_L    = 0x30000000;
  static constexpr uint32_t AddressOfFPGATypeRegister_H    = 0x30000002;
  static constexpr uint32_t AddressOfFPGAVersionRegister_L = 0x30000004;
  static constexpr uint32_t AddressOfFPGAVersionRegister_H = 0x30000006;
  // clang-format on

  // Clock Frequency
  static constexpr double ClockFrequency = 100;  // MHz
  // Clock Interval
  static constexpr double ClockInterval = 10e-9;  // s
  // FPGA TimeTag
  static constexpr uint32_t TimeTagResolutionInNanoSec = 10;  // ns
  // PHA Min/Max
  static constexpr uint16_t PHAMinimum = 0;
  static constexpr uint16_t PHAMaximum = 1023;

  std::unique_ptr<RMAPHandlerUART> rmapHandler;
  std::unique_ptr<RMAPInitiator> rmapIniaitorForGPSRegisterAccess;
  std::unique_ptr<EventDecoder> eventDecoder;
  std::unique_ptr<ChannelManager> channelManager;
  std::unique_ptr<ConsumerManagerEventFIFO> consumerManager;
  std::vector<std::unique_ptr<ChannelModule>> channelModules;

  size_t nReceivedEvents = 0;
  uint8_t gpsTimeRegister[LengthOfGPSTimeRegister + 1];
  const size_t GPSDataFIFODepthInBytes = 1024;
  uint8_t* gpsDataFIFOReadBuffer = NULL;
  std::vector<uint8_t> gpsDataFIFOData{};
  std::vector<GROWTH_FY2015_ADC_Type::Event*> events{};
  bool stopDumpThread_ = false;
};

#endif /* ADCBOARD_HH_ */
