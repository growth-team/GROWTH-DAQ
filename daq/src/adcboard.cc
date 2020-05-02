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

GROWTH_FY2015_ADCDumpThread::GROWTH_FY2015_ADCDumpThread(GROWTH_FY2015_ADC* parent) {
  this->parent       = parent;
  this->eventDecoder = parent->getEventDecoder();
}

void GROWTH_FY2015_ADCDumpThread::run() {
  CxxUtilities::Condition c;
  using namespace std;
  size_t nReceivedEvents_previous = 0;
  size_t delta                    = 0;
  size_t nReceivedEvents_latch;
  while (!this->stopped) {
    c.wait(WaitDurationInMilliSec);
    nReceivedEvents_latch    = parent->nReceivedEvents;
    delta                    = nReceivedEvents_latch - nReceivedEvents_previous;
    nReceivedEvents_previous = nReceivedEvents_latch;
    cout << "GROWTH_FY2015_ADC received " << dec << parent->nReceivedEvents << " events (delta=" << dec << delta << ")."
         << endl;
    cout << "GROWTH_FY2015_ADC_Type::EventDecoder available Event instances = " << dec
         << this->eventDecoder->getNAllocatedEventInstances() << endl;
  }
}

GROWTH_FY2015_ADC::GROWTH_FY2015_ADC(std::string deviceName) {
  using namespace std;

  cout << "#---------------------------------------------" << endl;
  cout << "# GROWTH_FY2015_ADC Constructor" << endl;
  cout << "#---------------------------------------------" << endl;
  // construct RMAPTargetNode instance
  adcRMAPTargetNode = new RMAPTargetNode;
  adcRMAPTargetNode->setID("ADCBox");
  adcRMAPTargetNode->setDefaultKey(0x00);
  adcRMAPTargetNode->setReplyAddress({});
  adcRMAPTargetNode->setTargetSpaceWireAddress({});
  adcRMAPTargetNode->setTargetLogicalAddress(0xFE);
  adcRMAPTargetNode->setInitiatorLogicalAddress(0xFE);

  rmapHandler    = new RMAPHandlerUART(deviceName, {adcRMAPTargetNode});
  bool connected = rmapHandler->connectoToSpaceWireToGigabitEther();
  if (!connected) {
    cerr << "SpaceWire interface could not be opened." << endl;
    ::exit(-1);
  } else {
    cout << "Connected to SpaceWire interface." << endl;
  }

  //
  rmapIniaitorForGPSRegisterAccess = new RMAPInitiator(rmapHandler->getRMAPEngine());

  // create an instance of ChannelManager
  this->channelManager = new ChannelManager(rmapHandler, adcRMAPTargetNode);

  // create an instance of ConsumerManager
  this->consumerManager = new ConsumerManagerEventFIFO(rmapHandler, adcRMAPTargetNode);

  // create instances of ADCChannelRegister
  for (size_t i = 0; i < SpaceFibreADC::NumberOfChannels; i++) {
    this->channelModules[i] = new ChannelModule(rmapHandler, adcRMAPTargetNode, i);
  }

  // event decoder
  this->eventDecoder = new EventDecoder();

  // dump thread
  /*
   this->dumpThread = new GROWTH_FY2015_ADCDumpThread(this);
   this->dumpThread->start();
   */
  cout << "Constructor completes." << endl;
}

GROWTH_FY2015_ADC::~GROWTH_FY2015_ADC() {
  using namespace std;
  cout << "GROWTH_FY2015_ADC::~GROWTH_FY2015_ADC(): Deconstructing GROWTH_FY2015_ADC instance." << endl;
  /*
   cout << "GROWTH_FY2015_ADC::~GROWTH_FY2015_ADC(): Stopping dump thread." << endl;
   if (this->dumpThread != NULL && this->dumpThread->isInRunMethod()) {
   this->dumpThread->stop();
   delete this->dumpThread;
   }*/

  cout << "GROWTH_FY2015_ADC::~GROWTH_FY2015_ADC(): Deleting RMAP Handler." << endl;
  delete rmapIniaitorForGPSRegisterAccess;
  delete rmapHandler;

  cout << "GROWTH_FY2015_ADC::~GROWTH_FY2015_ADC(): Deleting internal modules." << endl;
  delete this->channelManager;
  delete this->consumerManager;
  for (size_t i = 0; i < SpaceFibreADC::NumberOfChannels; i++) { delete this->channelModules[i]; }
  delete eventDecoder;

  cout << "GROWTH_FY2015_ADC::~GROWTH_FY2015_ADC(): Deleting GPS Data FIFO read buffer." << endl;
  if (gpsDataFIFOReadBuffer != NULL) { delete gpsDataFIFOReadBuffer; }

  cout << "GROWTH_FY2015_ADC::~GROWTH_FY2015_ADC(): Completed." << endl;
}

uint32_t GROWTH_FY2015_ADC::getFPGAType() {
  uint32_t fpgaType = rmapHandler->read32BitRegister(adcRMAPTargetNode, AddressOfFPGATypeRegister_L);
  return fpgaType;
}

uint32_t GROWTH_FY2015_ADC::getFPGAVersion() {
  using namespace std;
  uint32_t fpgaVersion = rmapHandler->read32BitRegister(adcRMAPTargetNode, AddressOfFPGAVersionRegister_L);
  return fpgaVersion;
}

std::string GROWTH_FY2015_ADC::getGPSRegister() {
  rmapHandler->read(adcRMAPTargetNode, AddressOfGPSTimeRegister, LengthOfGPSTimeRegister, gpsTimeRegister);
  std::stringstream ss;
  for (size_t i = 0; i < LengthOfGPSTimeRegister; i++) { ss << gpsTimeRegister[i]; }
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
  if (gpsDataFIFOData.size() != GPSDataFIFODepthInBytes) { gpsDataFIFOData.resize(GPSDataFIFODepthInBytes); }
  if (gpsDataFIFOReadBuffer == NULL) { gpsDataFIFOReadBuffer = new uint8_t[GPSDataFIFODepthInBytes]; }
  rmapHandler->read(adcRMAPTargetNode, AddressOfGPSDataFIFOResetRegister, GPSDataFIFODepthInBytes,
                    gpsDataFIFOReadBuffer);
  memcpy(&(gpsDataFIFOData[0]), gpsDataFIFOReadBuffer, GPSDataFIFODepthInBytes);
  return gpsDataFIFOData;
}

ChannelModule* GROWTH_FY2015_ADC::getChannelRegister(int chNumber) {
  using namespace std;
  if (Debug::adcbox()) { cout << "GROWTH_FY2015_ADC::getChannelRegister(" << chNumber << ")" << endl; }
  return channelModules[chNumber];
}

ChannelManager* GROWTH_FY2015_ADC::getChannelManager() {
  using namespace std;
  if (Debug::adcbox()) { cout << "GROWTH_FY2015_ADC::getChannelManager()" << endl; }
  return channelManager;
}

ConsumerManagerEventFIFO* GROWTH_FY2015_ADC::getConsumerManager() {
  using namespace std;
  if (Debug::adcbox()) { cout << "GROWTH_FY2015_ADC::getConsumerManager()" << endl; }
  return consumerManager;
}

void GROWTH_FY2015_ADC::reset() {
  using namespace std;
  if (Debug::adcbox()) { cout << "GROWTH_FY2015_ADC::reset()"; }
  this->channelManager->reset();
  this->consumerManager->reset();
  if (Debug::adcbox()) { cout << "done" << endl; }
}

void GROWTH_FY2015_ADC::closeDevice() {
  using namespace std;
  try {
    cout << "#stopping event data output" << endl;
    consumerManager->disableEventDataOutput();
    // cout << "#stopping dump thread" << endl;
    // consumerManager->stopDumpThread();
    // if (this->dumpThread != NULL) {
    //	this->dumpThread->stop();
    //}
    cout << "#closing sockets" << endl;
    consumerManager->closeSocket();
    cout << "#disconnecting SpaceWire-to-GigabitEther" << endl;
    rmapHandler->disconnectSpWGbE();
  } catch (...) { throw SpaceFibreADCException::CloseDeviceFailed; }
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
  for (auto event : events) { eventDecoder->freeEvent(event); }
}

void GROWTH_FY2015_ADC::setTriggerMode(size_t chNumber, TriggerMode triggerMode) {
  if (chNumber < SpaceFibreADC::NumberOfChannels) {
    channelModules[chNumber]->setTriggerMode(triggerMode);
  } else {
    using namespace std;
    cerr << "Error in setTriggerMode(): invalid channel number " << chNumber << endl;
    throw SpaceFibreADCException::InvalidChannelNumber;
  }
}

void GROWTH_FY2015_ADC::setNumberOfSamples(uint16_t nSamples) {
  for (size_t i = 0; i < SpaceFibreADC::NumberOfChannels; i++) { channelModules[i]->setNumberOfSamples(nSamples); }
  setNumberOfSamplesInEventPacket(nSamples);
}

void GROWTH_FY2015_ADC::setNumberOfSamplesInEventPacket(uint16_t nSamples) {
  consumerManager->setEventPacket_NumberOfWaveform(nSamples);
}

void GROWTH_FY2015_ADC::setStartingThreshold(size_t chNumber, uint32_t threshold) {
  if (chNumber < SpaceFibreADC::NumberOfChannels) {
    channelModules[chNumber]->setStartingThreshold(threshold);
  } else {
    using namespace std;
    cerr << "Error in setStartingThreshold(): invalid channel number " << chNumber << endl;
    throw SpaceFibreADCException::InvalidChannelNumber;
  }
}

void GROWTH_FY2015_ADC::setClosingThreshold(size_t chNumber, uint32_t threshold) {
  if (chNumber < SpaceFibreADC::NumberOfChannels) {
    channelModules[chNumber]->setClosingThreshold(threshold);
  } else {
    using namespace std;
    cerr << "Error in setClosingThreshold(): invalid channel number " << chNumber << endl;
    throw SpaceFibreADCException::InvalidChannelNumber;
  }
}

void GROWTH_FY2015_ADC::setTriggerBusMask(size_t chNumber, std::vector<size_t> enabledChannels) {
  if (chNumber < SpaceFibreADC::NumberOfChannels) {
    channelModules[chNumber]->setTriggerBusMask(enabledChannels);
  } else {
    using namespace std;
    cerr << "Error in setTriggerBusMask(): invalid channel number " << chNumber << endl;
    throw SpaceFibreADCException::InvalidChannelNumber;
  }
}

void GROWTH_FY2015_ADC::setDepthOfDelay(size_t chNumber, uint32_t depthOfDelay) {
  if (chNumber < SpaceFibreADC::NumberOfChannels) {
    channelModules[chNumber]->setDepthOfDelay(depthOfDelay);
  } else {
    using namespace std;
    cerr << "Error in setDepthOfDelay(): invalid channel number " << chNumber << endl;
    throw SpaceFibreADCException::InvalidChannelNumber;
  }
}

uint32_t GROWTH_FY2015_ADC::getLivetime(size_t chNumber) {
  if (chNumber < SpaceFibreADC::NumberOfChannels) {
    return channelModules[chNumber]->getLivetime();
  } else {
    using namespace std;
    cerr << "Error in getLivetime(): invalid channel number " << chNumber << endl;
    throw SpaceFibreADCException::InvalidChannelNumber;
  }
}

uint32_t GROWTH_FY2015_ADC::getCurrentADCValue(size_t chNumber) {
  if (chNumber < SpaceFibreADC::NumberOfChannels) {
    return channelModules[chNumber]->getCurrentADCValue();
  } else {
    using namespace std;
    cerr << "Error in getCurrentADCValue(): invalid channel number " << chNumber << endl;
    throw SpaceFibreADCException::InvalidChannelNumber;
  }
}

void GROWTH_FY2015_ADC::turnOnADCPower(size_t chNumber) {
  if (chNumber < SpaceFibreADC::NumberOfChannels) {
    channelModules[chNumber]->turnADCPower(true);
  } else {
    using namespace std;
    cerr << "Error in turnOnADCPower(): invalid channel number " << chNumber << endl;
    throw SpaceFibreADCException::InvalidChannelNumber;
  }
}

void GROWTH_FY2015_ADC::turnOffADCPower(size_t chNumber) {
  if (chNumber < SpaceFibreADC::NumberOfChannels) {
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
  if (chNumber < SpaceFibreADC::NumberOfChannels) {
    channelModules[chNumber]->sendCPUTrigger();
  } else {
    using namespace std;
    cerr << "Error in sendCPUTrigger(): invalid channel number " << chNumber << endl;
    throw SpaceFibreADCException::InvalidChannelNumber;
  }
}

void GROWTH_FY2015_ADC::sendCPUTrigger() {
  using namespace std;
  for (size_t chNumber = 0; chNumber < SpaceFibreADC::NumberOfChannels; chNumber++) {
    if (this->ChannelEnable[chNumber] == true) {  // if enabled
      cout << "CPU Trigger to Channel " << chNumber << endl;
      channelModules[chNumber]->sendCPUTrigger();
    }
  }
}

void GROWTH_FY2015_ADC::setPresetMode(SpaceFibreADC::PresetMode presetMode) {
  channelManager->setPresetMode(presetMode);
}

void GROWTH_FY2015_ADC::setPresetLivetime(uint32_t livetimeIn10msUnit) {
  channelManager->setPresetLivetime(livetimeIn10msUnit);
}

void GROWTH_FY2015_ADC::setPresetnEvents(uint32_t nEvents) { channelManager->setPresetnEvents(nEvents); }

double GROWTH_FY2015_ADC::getRealtime() { return channelManager->getRealtime(); }

void GROWTH_FY2015_ADC::setAdcClock(SpaceFibreADC::ADCClockFrequency adcClockFrequency) {
  channelManager->setAdcClock(adcClockFrequency);
}

SpaceFibreADC::HouseKeepingData GROWTH_FY2015_ADC::getHouseKeepingData() {
  SpaceFibreADC::HouseKeepingData hkData;
  // realtime
  hkData.realtime = channelManager->getRealtime();

  for (size_t i = 0; i < SpaceFibreADC::NumberOfChannels; i++) {
    // livetime
    hkData.livetime[i] = channelModules[i]->getLivetime();
    // acquisition status
    hkData.acquisitionStarted[i] = channelManager->isAcquisitionCompleted(i);
  }

  return hkData;
}

EventDecoder* GROWTH_FY2015_ADC::getEventDecoder() { return eventDecoder; }

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

public:
void GROWTH_FY2015_ADC::loadConfigurationFile(std::string inputFileName) {
  using namespace std;
  YAML::Node yaml_root                       = YAML::LoadFile(inputFileName);
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
  this->DetectorID         = yaml_root["DetectorID"].as<std::string>();
  this->PreTriggerSamples  = yaml_root["PreTriggerSamples"].as<size_t>();
  this->PostTriggerSamples = yaml_root["PostTriggerSamples"].as<size_t>();
  {
    // Convert integer-type trigger mode to TriggerMode enum type
    const std::vector<size_t> triggerModeInt = yaml_root["TriggerModes"].as<std::vector<size_t>>();
    for (size_t i = 0; i < triggerModeInt.size(); i++) {
      this->TriggerModes[i] = static_cast<enum TriggerMode>(triggerModeInt.at(i));
    }
  }
  this->SamplesInEventPacket               = yaml_root["SamplesInEventPacket"].as<size_t>();
  this->DownSamplingFactorForSavedWaveform = yaml_root["DownSamplingFactorForSavedWaveform"].as<size_t>();
  this->ChannelEnable                      = yaml_root["ChannelEnable"].as<std::vector<bool>>();
  this->TriggerThresholds                  = yaml_root["TriggerThresholds"].as<std::vector<uint16_t>>();
  this->TriggerCloseThresholds             = yaml_root["TriggerCloseThresholds"].as<std::vector<uint16_t>>();

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
    for (const auto mode : this->TriggerModes) { triggerModeInt.push_back(static_cast<size_t>(mode)); }
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
      this->setAdcClock(SpaceFibreADC::ADCClockFrequency::ADCClock50MHz);

      // turn on ADC
      this->turnOnADCPower(ch);
    }
    cout << "Device configurtion done." << endl;
  } catch (...) {
    cerr << "Device configuration failed." << endl;
    ::exit(-1);
  }
}
