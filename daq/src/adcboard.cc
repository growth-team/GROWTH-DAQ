#include "adcboard.hh"

#include "GROWTH_FY2015_ADCModules/ChannelManager.hh"
#include "GROWTH_FY2015_ADCModules/ChannelModule.hh"
#include "GROWTH_FY2015_ADCModules/Constants.hh"
#include "GROWTH_FY2015_ADCModules/ConsumerManagerEventFIFO.hh"
#include "GROWTH_FY2015_ADCModules/Debug.hh"
#include "GROWTH_FY2015_ADCModules/EventDecoder.hh"
#include "GROWTH_FY2015_ADCModules/RMAPHandlerUART.hh"
#include "GROWTH_FY2015_ADCModules/SemaphoreRegister.hh"
#include "GROWTH_FY2015_ADCModules/Types.hh"

#include "yaml-cpp/yaml.h"

void GROWTH_FY2015_ADC::dumpThread() {
  CxxUtilities::Condition c;
  size_t nReceivedEvents_previous = 0;
  size_t delta = 0;
  size_t nReceivedEvents_latch;
  while (!stopDumpThread_) {
    c.wait(1000);  // ms
    nReceivedEvents_latch = nReceivedEvents;
    delta = nReceivedEvents_latch - nReceivedEvents_previous;
    nReceivedEvents_previous = nReceivedEvents_latch;
    printf("Received %zu events (delta=%zu). Available Event instances=%zu\n", nReceivedEvents_latch, delta,
           eventDecoder->getNAllocatedEventInstances());
  }
}

GROWTH_FY2015_ADC::GROWTH_FY2015_ADC(std::string deviceName) : eventDecoder(new EventDecoder) {
  using namespace std;

  cout << "#---------------------------------------------" << endl;
  cout << "# GROWTH_FY2015_ADC Constructor" << endl;
  cout << "#---------------------------------------------" << endl;
  // construct RMAPTargetNode instance
  std::shared_ptr<RMAPTargetNode> adcRMAPTargetNode(new RMAPTargetNode);
  adcRMAPTargetNode->setID("ADCBox");
  adcRMAPTargetNode->setDefaultKey(0x00);
  adcRMAPTargetNode->setReplyAddress({});
  adcRMAPTargetNode->setTargetSpaceWireAddress({});
  adcRMAPTargetNode->setTargetLogicalAddress(0xFE);
  adcRMAPTargetNode->setInitiatorLogicalAddress(0xFE);

  rmapHandler.reset(new RMAPHandlerUART(deviceName, {adcRMAPTargetNode}));
  bool connected = rmapHandler->connectoToSpaceWireToGigabitEther();
  if (!connected) {
    cerr << "SpaceWire interface could not be opened." << endl;
    ::exit(-1);
  } else {
    cout << "Connected to SpaceWire interface." << endl;
  }

  //
  rmapIniaitorForGPSRegisterAccess.reset(new RMAPInitiator(rmapHandler->getRMAPEngine()));

  channelManager.reset(new ChannelManager(rmapHandler));
  consumerManager.reset(new ConsumerManagerEventFIFO(rmapHandler));

  // create instances of ADCChannelRegister
  for (size_t i = 0; i < GROWTH_FY2015_ADC_Type::NumberOfChannels; i++) {
    channelModules.emplace_back(rmapHandler, i);
  }
}

GROWTH_FY2015_ADC::~GROWTH_FY2015_ADC() {
  delete rmapIniaitorForGPSRegisterAccess;
  delete gpsDataFIFOReadBuffer;
}

uint32_t GROWTH_FY2015_ADC::getFPGAType() {
  return rmapHandler->read32BitRegister(adcRMAPTargetNode, AddressOfFPGATypeRegister_L);
}

uint32_t GROWTH_FY2015_ADC::getFPGAVersion() {
  using namespace std;
  uint32_t fpgaVersion = rmapHandler->read32BitRegister(adcRMAPTargetNode, AddressOfFPGAVersionRegister_L);
  return fpgaVersion;
}

std::string GROWTH_FY2015_ADC::getGPSRegister() {
  rmapHandler->read(adcRMAPTargetNode, AddressOfGPSTimeRegister, LengthOfGPSTimeRegister, gpsTimeRegister);
  std::stringstream ss;
  for (size_t i = 0; i < LengthOfGPSTimeRegister; i++) {
    ss << gpsTimeRegister[i];
  }
  return ss.str();
}

uint8_t* GROWTH_FY2015_ADC::getGPSRegisterUInt8() {
  rmapHandler->read(adcRMAPTargetNode, AddressOfGPSTimeRegister, LengthOfGPSTimeRegister, gpsTimeRegister);
  gpsTimeRegister[LengthOfGPSTimeRegister] = 0x00;
  return gpsTimeRegister;
}

void GROWTH_FY2015_ADC::clearGPSDataFIFO() {
  uint8_t dummy[2];
  rmapHandler->read(adcRMAPTargetNode, AddressOfGPSDataFIFOResetRegister, 2, dummy);
}

std::vector<uint8_t> GROWTH_FY2015_ADC::readGPSDataFIFO() {
  if (gpsDataFIFOData.size() != GPSDataFIFODepthInBytes) {
    gpsDataFIFOData.resize(GPSDataFIFODepthInBytes);
  }
  if (gpsDataFIFOReadBuffer == NULL) {
    gpsDataFIFOReadBuffer = new uint8_t[GPSDataFIFODepthInBytes];
  }
  rmapHandler->read(adcRMAPTargetNode, AddressOfGPSDataFIFOResetRegister, GPSDataFIFODepthInBytes,
                    gpsDataFIFOReadBuffer);
  memcpy(&(gpsDataFIFOData[0]), gpsDataFIFOReadBuffer, GPSDataFIFODepthInBytes);
  return gpsDataFIFOData;
}

void GROWTH_FY2015_ADC::reset() {
  using namespace std;
  if (Debug::adcbox()) {
    cout << "GROWTH_FY2015_ADC::reset()";
  }
  this->channelManager->reset();
  this->consumerManager->reset();
  if (Debug::adcbox()) {
    cout << "done" << endl;
  }
}

void GROWTH_FY2015_ADC::closeDevice() {
  using namespace std;
  try {
    rmapHandler->disconnectSpWGbE();
  } catch (...) {
    throw SpaceFibreADCException::CloseDeviceFailed;
  }
}

std::vector<GROWTH_FY2015_ADC_Type::Event*> GROWTH_FY2015_ADC::getEvent() {
  events.clear();
  std::vector<uint8_t> data = consumerManager->getEventData();
  if (data.size() != 0) {
    eventDecoder->decodeEvent(&data);
    events = eventDecoder->getDecodedEvents();
  }
  nReceivedEvents += events.size();
  return events;
}

void GROWTH_FY2015_ADC::freeEvent(GROWTH_FY2015_ADC_Type::Event* event) { eventDecoder->freeEvent(event); }

void GROWTH_FY2015_ADC::freeEvents(std::vector<GROWTH_FY2015_ADC_Type::Event*>& events) {
  for (auto event : events) {
    eventDecoder->freeEvent(event);
  }
}

void GROWTH_FY2015_ADC::setTriggerMode(size_t chNumber, TriggerMode triggerMode) {
  if (chNumber < GROWTH_FY2015_ADC_Type::NumberOfChannels) {
    channelModules[chNumber]->setTriggerMode(triggerMode);
  } else {
    using namespace std;
    cerr << "Error in setTriggerMode(): invalid channel number " << chNumber << endl;
    throw SpaceFibreADCException::InvalidChannelNumber;
  }
}

void GROWTH_FY2015_ADC::setNumberOfSamples(uint16_t nSamples) {
  for (size_t i = 0; i < GROWTH_FY2015_ADC_Type::NumberOfChannels; i++) {
    channelModules[i]->setNumberOfSamples(nSamples);
  }
  setNumberOfSamplesInEventPacket(nSamples);
}

void GROWTH_FY2015_ADC::setNumberOfSamplesInEventPacket(uint16_t nSamples) {
  consumerManager->setEventPacket_NumberOfWaveform(nSamples);
}

void GROWTH_FY2015_ADC::setStartingThreshold(size_t chNumber, uint32_t threshold) {
  if (chNumber < GROWTH_FY2015_ADC_Type::NumberOfChannels) {
    channelModules[chNumber]->setStartingThreshold(threshold);
  } else {
    using namespace std;
    cerr << "Error in setStartingThreshold(): invalid channel number " << chNumber << endl;
    throw SpaceFibreADCException::InvalidChannelNumber;
  }
}

void GROWTH_FY2015_ADC::setClosingThreshold(size_t chNumber, uint32_t threshold) {
  if (chNumber < GROWTH_FY2015_ADC_Type::NumberOfChannels) {
    channelModules[chNumber]->setClosingThreshold(threshold);
  } else {
    using namespace std;
    cerr << "Error in setClosingThreshold(): invalid channel number " << chNumber << endl;
    throw SpaceFibreADCException::InvalidChannelNumber;
  }
}

void GROWTH_FY2015_ADC::setDepthOfDelay(size_t chNumber, uint32_t depthOfDelay) {
  if (chNumber < GROWTH_FY2015_ADC_Type::NumberOfChannels) {
    channelModules[chNumber]->setDepthOfDelay(depthOfDelay);
  } else {
    using namespace std;
    cerr << "Error in setDepthOfDelay(): invalid channel number " << chNumber << endl;
    throw SpaceFibreADCException::InvalidChannelNumber;
  }
}

uint32_t GROWTH_FY2015_ADC::getLivetime(size_t chNumber) {
  if (chNumber < GROWTH_FY2015_ADC_Type::NumberOfChannels) {
    return channelModules[chNumber]->getLivetime();
  } else {
    using namespace std;
    cerr << "Error in getLivetime(): invalid channel number " << chNumber << endl;
    throw SpaceFibreADCException::InvalidChannelNumber;
  }
}

uint32_t GROWTH_FY2015_ADC::getCurrentADCValue(size_t chNumber) {
  if (chNumber < GROWTH_FY2015_ADC_Type::NumberOfChannels) {
    return channelModules[chNumber]->getCurrentADCValue();
  } else {
    using namespace std;
    cerr << "Error in getCurrentADCValue(): invalid channel number " << chNumber << endl;
    throw SpaceFibreADCException::InvalidChannelNumber;
  }
}

void GROWTH_FY2015_ADC::turnOnADCPower(size_t chNumber) {
  if (chNumber < GROWTH_FY2015_ADC_Type::NumberOfChannels) {
    channelModules[chNumber]->turnADCPower(true);
  } else {
    using namespace std;
    cerr << "Error in turnOnADCPower(): invalid channel number " << chNumber << endl;
    throw SpaceFibreADCException::InvalidChannelNumber;
  }
}

void GROWTH_FY2015_ADC::turnOffADCPower(size_t chNumber) {
  if (chNumber < GROWTH_FY2015_ADC_Type::NumberOfChannels) {
    channelModules[chNumber]->turnADCPower(false);
  } else {
    using namespace std;
    cerr << "Error in turnOffADCPower(): invalid channel number " << chNumber << endl;
    throw SpaceFibreADCException::InvalidChannelNumber;
  }
}

void GROWTH_FY2015_ADC::startAcquisition(std::vector<bool> channelsToBeStarted) {
  consumerManager->disableEventDataOutput();
  consumerManager->reset();  // reset EventFIFO
  consumerManager->enableEventDataOutput();
  channelManager->startAcquisition(channelsToBeStarted);
}

void GROWTH_FY2015_ADC::startAcquisition() {
  consumerManager->enableEventDataOutput();
  channelManager->startAcquisition(this->ChannelEnable);
}

bool GROWTH_FY2015_ADC::isAcquisitionCompleted() { return channelManager->isAcquisitionCompleted(); }

bool GROWTH_FY2015_ADC::isAcquisitionCompleted(size_t chNumber) {
  return channelManager->isAcquisitionCompleted(chNumber);
}

void GROWTH_FY2015_ADC::stopAcquisition() { channelManager->stopAcquisition(); }

void GROWTH_FY2015_ADC::sendCPUTrigger(size_t chNumber) {
  if (chNumber < GROWTH_FY2015_ADC_Type::NumberOfChannels) {
    channelModules[chNumber]->sendCPUTrigger();
  } else {
    using namespace std;
    cerr << "Error in sendCPUTrigger(): invalid channel number " << chNumber << endl;
    throw SpaceFibreADCException::InvalidChannelNumber;
  }
}

void GROWTH_FY2015_ADC::sendCPUTrigger() {
  using namespace std;
  for (size_t chNumber = 0; chNumber < GROWTH_FY2015_ADC_Type::NumberOfChannels; chNumber++) {
    if (this->ChannelEnable[chNumber] == true) {  // if enabled
      cout << "CPU Trigger to Channel " << chNumber << endl;
      channelModules[chNumber]->sendCPUTrigger();
    }
  }
}

void GROWTH_FY2015_ADC::setPresetMode(GROWTH_FY2015_ADC_Type::PresetMode presetMode) {
  channelManager->setPresetMode(presetMode);
}

void GROWTH_FY2015_ADC::setPresetLivetime(uint32_t livetimeIn10msUnit) {
  channelManager->setPresetLivetime(livetimeIn10msUnit);
}

void GROWTH_FY2015_ADC::setPresetnEvents(uint32_t nEvents) { channelManager->setPresetnEvents(nEvents); }

double GROWTH_FY2015_ADC::getRealtime() { return channelManager->getRealtime(); }

void GROWTH_FY2015_ADC::setAdcClock(GROWTH_FY2015_ADC_Type::ADCClockFrequency adcClockFrequency) {
  channelManager->setAdcClock(adcClockFrequency);
}

GROWTH_FY2015_ADC_Type::HouseKeepingData GROWTH_FY2015_ADC::getHouseKeepingData() {
  GROWTH_FY2015_ADC_Type::HouseKeepingData hkData;
  // realtime
  hkData.realtime = channelManager->getRealtime();

  for (size_t i = 0; i < GROWTH_FY2015_ADC_Type::NumberOfChannels; i++) {
    // livetime
    hkData.livetime[i] = channelModules[i]->getLivetime();
    // acquisition status
    hkData.acquisitionStarted[i] = channelManager->isAcquisitionCompleted(i);
  }

  return hkData;
}

size_t GROWTH_FY2015_ADC::getNSamplesInEventListFile() {
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

void GROWTH_FY2015_ADC::loadConfigurationFile(std::string inputFileName) {
  using namespace std;
  YAML::Node yaml_root = YAML::LoadFile(inputFileName);
  std::vector<std::string> mustExistKeywords = {"DetectorID",
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
  this->TriggerThresholds = yaml_root["TriggerThresholds"].as<std::vector<uint16_t>>();
  this->TriggerCloseThresholds = yaml_root["TriggerCloseThresholds"].as<std::vector<uint16_t>>();

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
    cout << CxxUtilities::String::join(triggerModeInt, ", ") << "]" << endl;
  }
  cout << "SamplesInEventPacket              : " << this->SamplesInEventPacket << endl;
  cout << "DownSamplingFactorForSavedWaveform: " << this->DownSamplingFactorForSavedWaveform << endl;
  cout << "ChannelEnable                     : [" << CxxUtilities::String::join(this->ChannelEnable, ", ") << "]"
       << endl;
  cout << "TriggerThresholds                 : [" << CxxUtilities::String::join(this->TriggerThresholds, ", ") << "]"
       << endl;
  cout << "TriggerCloseThresholds            : [" << CxxUtilities::String::join(this->TriggerCloseThresholds, ", ")
       << "]" << endl;
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
      this->setAdcClock(GROWTH_FY2015_ADC_Type::ADCClockFrequency::ADCClock50MHz);

      // turn on ADC
      this->turnOnADCPower(ch);
    }
    cout << "Device configuration done." << endl;
  } catch (...) {
    cerr << "Device configuration failed." << endl;
    ::exit(-1);
  }
}
