#include "adcboard.hh"

#include "stringutil.hh"
#include "yaml-cpp/yaml.h"

#include "growth-fpga/channelmanager.hh"
#include "growth-fpga/channelmodule.hh"
#include "growth-fpga/constants.hh"
#include "growth-fpga/consumermanagereventfifo.hh"
#include "growth-fpga/eventdecoder.hh"
#include "growth-fpga/registeraccessinterface.hh"
#include "growth-fpga/rmaphandleruart.hh"
#include "growth-fpga/semaphoreregister.hh"
#include "growth-fpga/types.hh"

#include <cassert>
#include <chrono>


void GROWTH_FY2015_ADC::dumpThread() {
  size_t nReceivedEvents_previous = 0;
  size_t delta = 0;
  size_t nReceivedEvents_latch;
  while (!stopDumpThread_) {
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(1s);
    nReceivedEvents_latch = nReceivedEvents_;
    delta = nReceivedEvents_latch - nReceivedEvents_previous;
    nReceivedEvents_previous = nReceivedEvents_latch;
    printf("Received %zu events (delta=%zu). Available Event instances=%zu\n", nReceivedEvents_latch, delta,
           eventDecoder_->getNAllocatedEventInstances());
  }
}

GROWTH_FY2015_ADC::GROWTH_FY2015_ADC(std::string deviceName)
    : eventDecoder_(new EventDecoder),
      rmapHandler_(std::make_unique<RMAPHandlerUART>(deviceName)),
      rmapIniaitorForGPSRegisterAccess_(std::make_unique<RMAPInitiator>(rmapHandler_->getRMAPEngine())),
      gpsDataFIFOReadBuffer_(new u8[GPS_DATA_FIFO_DEPTH_BYTES]) {
  adcRMAPTargetNode_ = std::make_shared<RMAPTargetNode>();
  adcRMAPTargetNode_->setDefaultKey(0x00);
  adcRMAPTargetNode_->setReplyAddress({});
  adcRMAPTargetNode_->setTargetSpaceWireAddress({});
  adcRMAPTargetNode_->setTargetLogicalAddress(0xFE);
  adcRMAPTargetNode_->setInitiatorLogicalAddress(0xFE);

  channelManager_ = std::make_unique<ChannelManager>(rmapHandler_, adcRMAPTargetNode_);
  consumerManager_ = std::make_unique<ConsumerManagerEventFIFO>(rmapHandler_, adcRMAPTargetNode_);

  // create instances of ADCChannelRegister
  for (size_t i = 0; i < growth_fpga::NumberOfChannels; i++) {
    channelModules_.emplace_back(new ChannelModule(rmapHandler_, adcRMAPTargetNode_, i));
  }

  reg_ = std::make_shared<RegisterAccessInterface>(rmapHandler_, adcRMAPTargetNode_);
}

GROWTH_FY2015_ADC::~GROWTH_FY2015_ADC() = default;

u32 GROWTH_FY2015_ADC::getFPGAType() const { return reg_->read32(AddressOfFPGATypeRegister_L); }

u32 GROWTH_FY2015_ADC::getFPGAVersion() const { return reg_->read32(AddressOfFPGAVersionRegister_L); }

std::string GROWTH_FY2015_ADC::getGPSRegister() const {
  std::array<u8, GPS_TIME_REG_SIZE_BYTES + 1> gpsTimeRegister{};
  reg_->read(AddressOfGPSTimeRegister, GPS_TIME_REG_SIZE_BYTES, gpsTimeRegister.data());
  std::stringstream ss;
  for (size_t i = 0; i < GPS_TIME_REG_SIZE_BYTES; i++) {
    if (i < 14) {
      ss << gpsTimeRegister.at(i);
    } else {
      ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<u32>(gpsTimeRegister.at(i));
    }
    if (i == 13) {
      ss << " ";
    }
  }
  return ss.str();
}

std::array<u8, GROWTH_FY2015_ADC::GPS_TIME_REG_SIZE_BYTES + 1> GROWTH_FY2015_ADC::getGPSRegisterUInt8() const {
  std::array<u8, GPS_TIME_REG_SIZE_BYTES + 1> gpsTimeRegister{};
  reg_->read(AddressOfGPSTimeRegister, GPS_TIME_REG_SIZE_BYTES, gpsTimeRegister.data());
  gpsTimeRegister.at(GPS_TIME_REG_SIZE_BYTES) = 0x00;
  return gpsTimeRegister;
}

void GROWTH_FY2015_ADC::clearGPSDataFIFO() {
  u8 dummy[2];
  reg_->read(AddressOfGPSDataFIFOResetRegister, 2, dummy);
}

std::vector<u8> GROWTH_FY2015_ADC::readGPSDataFIFO() {
  if (gpsDataFIFOData_.size() != GPS_DATA_FIFO_DEPTH_BYTES) {
    gpsDataFIFOData_.resize(GPS_DATA_FIFO_DEPTH_BYTES);
  }
  reg_->read(AddressOfGPSDataFIFOResetRegister, GPS_DATA_FIFO_DEPTH_BYTES, gpsDataFIFOReadBuffer_.get());
  memcpy(gpsDataFIFOData_.data(), gpsDataFIFOReadBuffer_.get(), GPS_DATA_FIFO_DEPTH_BYTES);
  return gpsDataFIFOData_;
}

void GROWTH_FY2015_ADC::reset() {
  channelManager_->reset();
  consumerManager_->reset();
}

std::vector<growth_fpga::Event*> GROWTH_FY2015_ADC::getEvent() {
  events_.clear();
  const std::vector<u8> data = consumerManager_->getEventData();
  if (!data.empty()) {
    eventDecoder_->decodeEvent(&data);
    events_ = eventDecoder_->getDecodedEvents();
  }
  nReceivedEvents_ += events_.size();
  return events_;
}

void GROWTH_FY2015_ADC::freeEvent(growth_fpga::Event* event) { eventDecoder_->freeEvent(event); }

void GROWTH_FY2015_ADC::freeEvents(std::vector<growth_fpga::Event*>& events) {
  for (auto event : events) {
    eventDecoder_->freeEvent(event);
  }
}

void GROWTH_FY2015_ADC::setTriggerMode(size_t chNumber, TriggerMode triggerMode) {
  assert(chNumber < growth_fpga::NumberOfChannels);
  channelModules_[chNumber]->setTriggerMode(triggerMode);
}

void GROWTH_FY2015_ADC::setNumberOfSamples(u16 nSamples) {
  for (size_t i = 0; i < growth_fpga::NumberOfChannels; i++) {
    channelModules_[i]->setNumberOfSamples(nSamples);
  }
  setNumberOfSamplesInEventPacket(nSamples);
}

void GROWTH_FY2015_ADC::setNumberOfSamplesInEventPacket(u16 nSamples) {
  consumerManager_->setEventPacket_NumberOfWaveform(nSamples);
}

void GROWTH_FY2015_ADC::setStartingThreshold(size_t chNumber, u32 threshold) {
  assert(chNumber < growth_fpga::NumberOfChannels);
  channelModules_[chNumber]->setStartingThreshold(threshold);
}

void GROWTH_FY2015_ADC::setClosingThreshold(size_t chNumber, u32 threshold) {
  assert(chNumber < growth_fpga::NumberOfChannels);
  channelModules_[chNumber]->setClosingThreshold(threshold);
}

void GROWTH_FY2015_ADC::setDepthOfDelay(size_t chNumber, u32 depthOfDelay) {
  assert(chNumber < growth_fpga::NumberOfChannels);
  channelModules_[chNumber]->setDepthOfDelay(depthOfDelay);
}

u32 GROWTH_FY2015_ADC::getLivetime(size_t chNumber) const {
  assert(chNumber < growth_fpga::NumberOfChannels);
  return channelModules_[chNumber]->getLivetime();
}

u32 GROWTH_FY2015_ADC::getCurrentADCValue(size_t chNumber) const {
  assert(chNumber < growth_fpga::NumberOfChannels);
  return channelModules_[chNumber]->getCurrentADCValue();
}

void GROWTH_FY2015_ADC::turnOnADCPower(size_t chNumber) {
  assert(chNumber < growth_fpga::NumberOfChannels);
  channelModules_[chNumber]->turnADCPower(true);
}

void GROWTH_FY2015_ADC::turnOffADCPower(size_t chNumber) {
  assert(chNumber < growth_fpga::NumberOfChannels);
  channelModules_[chNumber]->turnADCPower(false);
}

void GROWTH_FY2015_ADC::startAcquisition(std::vector<bool> channelsToBeStarted) {
  consumerManager_->reset();  // reset EventFIFO
  channelManager_->startAcquisition(channelsToBeStarted);
}

void GROWTH_FY2015_ADC::startAcquisition() { channelManager_->startAcquisition(this->ChannelEnable); }

bool GROWTH_FY2015_ADC::isAcquisitionCompleted() const { return channelManager_->isAcquisitionCompleted(); }

bool GROWTH_FY2015_ADC::isAcquisitionCompleted(size_t chNumber) const {
  return channelManager_->isAcquisitionCompleted(chNumber);
}

void GROWTH_FY2015_ADC::stopAcquisition() { channelManager_->stopAcquisition(); }

void GROWTH_FY2015_ADC::sendCPUTrigger(size_t chNumber) {
  if (chNumber < growth_fpga::NumberOfChannels) {
    channelModules_[chNumber]->sendCPUTrigger();
  } else {
    throw SpaceFibreADCException::InvalidChannelNumber;
  }
}

void GROWTH_FY2015_ADC::sendCPUTrigger() {
  for (size_t chNumber = 0; chNumber < growth_fpga::NumberOfChannels; chNumber++) {
    if (this->ChannelEnable[chNumber]) {  // if enabled
      std::cout << "CPU Trigger to Channel " << chNumber << std::endl;
      channelModules_[chNumber]->sendCPUTrigger();
    }
  }
}

void GROWTH_FY2015_ADC::setPresetMode(growth_fpga::PresetMode presetMode) {
  channelManager_->setPresetMode(presetMode);
}

void GROWTH_FY2015_ADC::setPresetLivetime(u32 livetimeIn10msUnit) {
  channelManager_->setPresetLivetime(livetimeIn10msUnit);
}

void GROWTH_FY2015_ADC::setPresetnEvents(u32 nEvents) { channelManager_->setPresetnEvents(nEvents); }

f64 GROWTH_FY2015_ADC::getRealtime() const { return channelManager_->getRealtime(); }

void GROWTH_FY2015_ADC::setAdcClock(growth_fpga::ADCClockFrequency adcClockFrequency) {
  channelManager_->setAdcClock(adcClockFrequency);
}

growth_fpga::HouseKeepingData GROWTH_FY2015_ADC::getHouseKeepingData() const {
  growth_fpga::HouseKeepingData hkData{};
  hkData.realtime = channelManager_->getRealtime();

  for (size_t i = 0; i < growth_fpga::NumberOfChannels; i++) {
    // livetime
    hkData.livetime[i] = channelModules_[i]->getLivetime();
    // acquisition status
    hkData.acquisitionStarted[i] = channelManager_->isAcquisitionCompleted(i);
  }

  return hkData;
}

size_t GROWTH_FY2015_ADC::getNSamplesInEventListFile() const {
  return (this->SamplesInEventPacket) / this->DownSamplingFactorForSavedWaveform;
}

void GROWTH_FY2015_ADC::dumpMustExistKeywords() {
  using namespace std;
  cout << "---------------------------------------------" << endl;
  cout << "The following is a template of a configuration file." << endl;
  cout << "---------------------------------------------" << endl;
  cout << endl;
  cout                                                      //
      << "DetectorID: fy2015a" << endl                      //
      << "PreTriggerSamples: 10" << endl                    //
      << "PostTriggerSamples: 500" << endl                  //
      << "SamplesInEventPacket: 510" << endl                //
      << "DownSamplingFactorForSavedWaveform: 4" << endl    //
      << "ChannelEnable: [true, true, true, true]" << endl  //
      << "TriggerThresholds: [800, 800, 800, 800]" << endl  //
      << "TriggerCloseThresholds: [800, 800, 800, 800]" << endl;
}

void GROWTH_FY2015_ADC::loadConfigurationFile(const std::string& inputFileName) {
  using namespace std;
  YAML::Node yaml_root = YAML::LoadFile(inputFileName);
  const std::vector<std::string> mustExistKeywords = {"DetectorID",
                                                      "PreTriggerSamples",
                                                      "PostTriggerSamples",
                                                      "SamplesInEventPacket",
                                                      "DownSamplingFactorForSavedWaveform",
                                                      "ChannelEnable",
                                                      "TriggerThresholds",
                                                      "TriggerCloseThresholds"};

  //---------------------------------------------
  // check keyword existence
  //---------------------------------------------
  for (auto keyword : mustExistKeywords) {
    if (!yaml_root[keyword].IsDefined()) {
      cerr << "Error: " << keyword << " is not defined in the configuration file." << endl;
      dumpMustExistKeywords();
      exit(-1);
    }
  }

  //---------------------------------------------
  // load parameter values from the file
  //---------------------------------------------
  this->DetectorID = yaml_root["DetectorID"].as<std::string>();
  this->PreTriggerSamples = yaml_root["PreTriggerSamples"].as<size_t>();
  this->PostTriggerSamples = yaml_root["PostTriggerSamples"].as<size_t>();
  {
    // Convert integer-type trigger mode to TriggerMode enum type
    const std::vector<size_t> triggerModeInt = yaml_root["TriggerModes"].as<std::vector<size_t>>();
    for (size_t i = 0; i < triggerModeInt.size(); i++) {
      this->TriggerModes[i] = static_cast<enum TriggerMode>(triggerModeInt.at(i));
    }
  }
  this->SamplesInEventPacket = yaml_root["SamplesInEventPacket"].as<size_t>();
  this->DownSamplingFactorForSavedWaveform = yaml_root["DownSamplingFactorForSavedWaveform"].as<size_t>();
  this->ChannelEnable = yaml_root["ChannelEnable"].as<std::vector<bool>>();
  this->TriggerThresholds = yaml_root["TriggerThresholds"].as<std::vector<u16>>();
  this->TriggerCloseThresholds = yaml_root["TriggerCloseThresholds"].as<std::vector<u16>>();

  //---------------------------------------------
  // dump setting
  //---------------------------------------------
  cout << "#---------------------------------------------" << endl;
  cout << "# Configuration" << endl;
  cout << "#---------------------------------------------" << endl;
  cout << "DetectorID                        : " << this->DetectorID << endl;
  cout << "PreTriggerSamples                 : " << this->PreTriggerSamples << endl;
  cout << "PostTriggerSamples                : " << this->PostTriggerSamples << endl;
  {
    cout << "TriggerModes                      : [";
    std::vector<size_t> triggerModeInt{};
    for (const auto mode : this->TriggerModes) {
      triggerModeInt.push_back(static_cast<size_t>(mode));
    }
    cout << stringutil::join(triggerModeInt, ", ") << "]" << endl;
  }
  cout << "SamplesInEventPacket              : " << this->SamplesInEventPacket << endl;
  cout << "DownSamplingFactorForSavedWaveform: " << this->DownSamplingFactorForSavedWaveform << endl;
  cout << "ChannelEnable                     : [" << stringutil::join(this->ChannelEnable, ", ") << "]" << endl;
  cout << "TriggerThresholds                 : [" << stringutil::join(this->TriggerThresholds, ", ") << "]" << endl;
  cout << "TriggerCloseThresholds            : [" << stringutil::join(this->TriggerCloseThresholds, ", ") << "]"
       << endl;
  cout << endl;

  cout << "//---------------------------------------------" << endl;
  cout << "// Programing the digitizer" << endl;
  cout << "//---------------------------------------------" << endl;
  try {
    // record length
    this->setNumberOfSamples(PreTriggerSamples + PostTriggerSamples);
    this->setNumberOfSamplesInEventPacket(SamplesInEventPacket);
    for (size_t ch = 0; ch < nChannels; ch++) {
      // pre-trigger (delay)
      this->setDepthOfDelay(ch, PreTriggerSamples);

      // trigger mode
      const auto triggerMode = this->TriggerModes.at(ch);
      this->setTriggerMode(ch, triggerMode);

      // threshold
      this->setStartingThreshold(ch, TriggerThresholds[ch]);
      this->setClosingThreshold(ch, TriggerCloseThresholds[ch]);

      // adc clock 50MHz
      this->setAdcClock(growth_fpga::ADCClockFrequency::ADCClock50MHz);

      // turn on ADC
      this->turnOnADCPower(ch);
    }
    cout << "Device configuration done." << endl;
  } catch (...) {
    cerr << "Device configuration failed." << endl;
    ::exit(-1);
  }
}
