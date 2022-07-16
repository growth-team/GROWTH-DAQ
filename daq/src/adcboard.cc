#include "adcboard.hh"

#include "stringutil.hh"
#include "yaml-cpp/yaml.h"
#include "spdlog/spdlog.h"

#include "growth-fpga/channelmanager.hh"
#include "growth-fpga/channelmodule.hh"
#include "growth-fpga/constants.hh"
#include "growth-fpga/consumermanagereventfifo.hh"
#include "growth-fpga/eventdecoder.hh"
#include "growth-fpga/registeraccessinterface.hh"
#include "growth-fpga/semaphoreregister.hh"
#include "growth-fpga/hitpatternmodule.hh"
#include "growth-fpga/slowadcdac.hh"
#include "growth-fpga/types.hh"

#include <cassert>
#include <chrono>

void GROWTH_FY2015_ADC::dumpThread(const std::chrono::seconds dumpInterval) {
  size_t nReceivedEvents_previous = 0;
  size_t delta = 0;
  size_t nReceivedEvents_latch;
  while (!stopDumpThread_) {
    nReceivedEvents_latch = nReceivedEvents_;
    delta = nReceivedEvents_latch - nReceivedEvents_previous;
    nReceivedEvents_previous = nReceivedEvents_latch;
    const auto rateHz = static_cast<f64>(delta) / dumpInterval.count();
    const auto elapsedTimeSec = [&]() -> f64 {
      if (acquisitionStartTime_) {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() -
                                                                     acquisitionStartTime_.value())
            .count();

      } else {
        return 0.0;
      }
    }() / 1000.0;

    spdlog::info(
        "Elapsed time = {:5.1f} sec, Received total = {:5} events, rate = {:4.1f} Hz, "
        "available event instances = {:5d}",  //
        elapsedTimeSec, nReceivedEvents_latch, rateHz, eventDecoder_->getNAllocatedEventInstances());
    std::unique_lock lock{dumpThreadWaitMutex_};
    const auto timeoutTime = std::chrono::system_clock::now() + dumpInterval;
    dumpThreadWaitCondition_.wait_until(lock, timeoutTime, [&]() { return stopDumpThread_.load(); });
  }
}

GROWTH_FY2015_ADC::GROWTH_FY2015_ADC(std::string deviceName)
    : eventDecoder_(std::make_unique<EventDecoder>()),
      gpsDataFIFOReadBuffer_(std::make_unique<u8[]>(GPS_DATA_FIFO_DEPTH_BYTES)) {
  spwif_ = std::make_unique<SpaceWireIFOverUART>(deviceName);
  const bool openResult = spwif_->open();
  assert(openResult);

  // Skip Data CRC check of reply packets sent from the board for faster processing.
  const RMAPEngine::ReceivedPacketOption receivedPacketOption{false, false};
  rmapEngine_ = std::make_shared<RMAPEngine>(spwif_.get(), receivedPacketOption);
  rmapEngine_->start();

  adcRMAPTargetNode_ = std::make_shared<RMAPTargetNode>();
  adcRMAPTargetNode_->setDefaultKey(0x00);

  channelManager_ = std::make_unique<ChannelManager>(std::make_shared<RMAPInitiator>(rmapEngine_), adcRMAPTargetNode_);
  consumerManager_ =
      std::make_unique<ConsumerManagerEventFIFO>(std::make_shared<RMAPInitiator>(rmapEngine_), adcRMAPTargetNode_);

  // create instances of ADCChannelRegister
  for (size_t i = 0; i < growth_fpga::NumberOfChannels; i++) {
    channelModules_.emplace_back(
        std::make_unique<ChannelModule>(std::make_shared<RMAPInitiator>(rmapEngine_), adcRMAPTargetNode_, i));
  }

  reg_ = std::make_shared<RegisterAccessInterface>(std::make_shared<RMAPInitiator>(rmapEngine_), adcRMAPTargetNode_);

  eventDecoder_->pauseEventDecoding();
  eventPacketReadThread_ = std::thread(&GROWTH_FY2015_ADC::eventPacketReadThread, this);
}

GROWTH_FY2015_ADC::~GROWTH_FY2015_ADC() {
  spdlog::info("ADCBoard: Destructor is called. Waiting for threads to join...");
  stopDumpThread_ = true;
  stopEventPacketReadThread_ = true;
  dumpThreadWaitCondition_.notify_one();
  dumpThread_.join();
  spdlog::info("ADCBoard: Dump thread joined");
  eventPacketReadThread_.join();
  spdlog::info("ADCBoard: EventPacketReadThread thread joined");
}

void GROWTH_FY2015_ADC::startDumpThread() {
  assert(!dumpThread_.joinable());
  dumpThread_ = std::thread(&GROWTH_FY2015_ADC::dumpThread, this, std::chrono::seconds(1));
}

u32 GROWTH_FY2015_ADC::getFPGAType() const { return reg_->read32(ADDRESS_FPGA_TYPE_REGISTER_L); }

u32 GROWTH_FY2015_ADC::getFPGAVersion() const { return reg_->read32(ADDRESS_FPGA_VERSION_REGISTER_L); }

std::string GROWTH_FY2015_ADC::getGPSRegister() const {
  std::array<u8, GPS_TIME_REG_SIZE_BYTES + 1> gpsTimeRegister{};
  reg_->read(ADDRESS_GPS_TIME_REGISTER, GPS_TIME_REG_SIZE_BYTES, gpsTimeRegister.data());
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
  reg_->read(ADDRESS_GPS_TIME_REGISTER, GPS_TIME_REG_SIZE_BYTES, gpsTimeRegister.data());
  gpsTimeRegister.at(GPS_TIME_REG_SIZE_BYTES) = 0x00;
  return gpsTimeRegister;
}

void GROWTH_FY2015_ADC::clearGPSDataFIFO() {
  u8 dummy[2];
  reg_->read(ADDRESS_GPS_DATA_FIFO_RESET_REGISTER, 2, dummy);
}

const std::vector<u8>& GROWTH_FY2015_ADC::readGPSDataFIFO() {
  if (gpsDataFIFOData_.size() != GPS_DATA_FIFO_DEPTH_BYTES) {
    gpsDataFIFOData_.resize(GPS_DATA_FIFO_DEPTH_BYTES);
  }
  reg_->read(ADDRESS_GPS_DATA_FIFO_RESET_REGISTER, GPS_DATA_FIFO_DEPTH_BYTES, gpsDataFIFOReadBuffer_.get());
  memcpy(gpsDataFIFOData_.data(), gpsDataFIFOReadBuffer_.get(), GPS_DATA_FIFO_DEPTH_BYTES);
  return gpsDataFIFOData_;
}

std::optional<u16> GROWTH_FY2015_ADC::readRegister16(const u32 address) {
  try {
    const u16 value = reg_->read16(address);
    return value;
  } catch (...) {
    return {};
  }
}

bool GROWTH_FY2015_ADC::writeRegister16(const u32 address, const u16 value) {
  try {
    reg_->write(address, value);
    return true;
  } catch (...) {
    return false;
  }
}

std::optional<u32> GROWTH_FY2015_ADC::readRegister32(const u32 address) {
  try {
    const u32 value = reg_->read32(address);
    return value;
  } catch (...) {
    return {};
  }
}

bool GROWTH_FY2015_ADC::writeRegister32(const u32 address, const u32 value) {
  try {
    reg_->write(address, value);
    return true;
  } catch (...) {
    return false;
  }
}

void GROWTH_FY2015_ADC::reset() {
  channelManager_->stopAcquisition();
  channelManager_->reset();
  eventDecoder_->pauseEventDecoding();
  consumerManager_->disableEventOutput();  // stop event recording in FIFO
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  eventDecoder_->reset();
  eventDecoder_->resumeEventDecoding();
  consumerManager_->enableEventOutput();  // start event recording in FIFO
}

void GROWTH_FY2015_ADC::eventPacketReadThread() {
  spdlog::info("EventPacketReadThread started...");
  U8BufferPtr u8BufferPtr{};
  while (!stopEventPacketReadThread_) {
    if (!u8BufferPtr) {
      u8BufferPtr = eventDecoder_->borrowU8Buffer();
      assert(u8BufferPtr);
    }
    consumerManager_->getEventData(*u8BufferPtr);
    if (!u8BufferPtr->empty()) {
      eventDecoder_->pushEventPacketData(std::move(u8BufferPtr));
      u8BufferPtr.reset();
    }
  }
  spdlog::info("EventPacketReadThread stopped.");
}

std::optional<EventListPtr> GROWTH_FY2015_ADC::getEventList() {
  auto result = eventDecoder_->getEventList();
  if (result) {
    nReceivedEvents_ += result.value()->size();
  }
  return result;
}

void GROWTH_FY2015_ADC::returnEventList(EventListPtr eventList) {
  eventDecoder_->returnEventList(std::move(eventList));
}

void GROWTH_FY2015_ADC::setTriggerMode(size_t chNumber, growth_fpga::TriggerMode triggerMode) {
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
  reset();
  eventDecoder_->resumeEventDecoding();
  acquisitionStartTime_ = std::chrono::system_clock::now();
  channelManager_->startAcquisition(channelsToBeStarted);
}

void GROWTH_FY2015_ADC::startAcquisition() { startAcquisition(acquisitionConfig_.channelEnable); }

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
    if (acquisitionConfig_.channelEnable.at(chNumber)) {
      spdlog::info("CPU Trigger to Channel {}", chNumber);
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
  return acquisitionConfig_.samplesInEventPacket / acquisitionConfig_.downSamplingFactorForSavedWaveform;
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
  YAML::Node yaml_root = YAML::LoadFile(inputFileName);
  const std::vector<std::string> mustExistKeywords = {"DetectorID",
                                                      "PreTriggerSamples",
                                                      "PostTriggerSamples",
                                                      "SamplesInEventPacket",
                                                      "DownSamplingFactorForSavedWaveform",
                                                      "ChannelEnable",
                                                      "TriggerModes",
                                                      "TriggerThresholds",
                                                      "TriggerCloseThresholds"};

  //---------------------------------------------
  // check keyword existence
  //---------------------------------------------
  for (auto keyword : mustExistKeywords) {
    if (!yaml_root[keyword].IsDefined()) {
      spdlog::error("Keyword {} is not defined in the configuration file.", keyword);
      dumpMustExistKeywords();
      throw std::runtime_error("Invalid configuration file format");
    }
  }

  //---------------------------------------------
  // load parameter values from the file
  //---------------------------------------------
  acquisitionConfig_.detectorID = yaml_root["DetectorID"].as<std::string>();
  acquisitionConfig_.preTriggerSamples = yaml_root["PreTriggerSamples"].as<size_t>();
  acquisitionConfig_.postTriggerSamples = yaml_root["PostTriggerSamples"].as<size_t>();
  {
    // Convert integer-type trigger mode to TriggerMode enum type
    const std::vector<size_t> triggerModeInt = yaml_root["TriggerModes"].as<std::vector<size_t>>();
    for (size_t i = 0; i < triggerModeInt.size(); i++) {
      acquisitionConfig_.triggerModes.at(i) = static_cast<enum growth_fpga::TriggerMode>(triggerModeInt.at(i));
    }
  }
  acquisitionConfig_.samplesInEventPacket = yaml_root["SamplesInEventPacket"].as<size_t>();
  acquisitionConfig_.downSamplingFactorForSavedWaveform = yaml_root["DownSamplingFactorForSavedWaveform"].as<size_t>();
  acquisitionConfig_.channelEnable = yaml_root["ChannelEnable"].as<std::vector<bool>>();
  acquisitionConfig_.triggerThresholds = yaml_root["TriggerThresholds"].as<std::vector<u16>>();
  acquisitionConfig_.triggerCloseThresholds = yaml_root["TriggerCloseThresholds"].as<std::vector<u16>>();

  dumpParameters();
  programDigitizer();
}

void GROWTH_FY2015_ADC::dumpParameters() {
  //---------------------------------------------
  // dump setting
  //---------------------------------------------
  spdlog::info("Configuration");
  spdlog::info("  DetectorID                        : {}", acquisitionConfig_.detectorID);
  spdlog::info("  PreTriggerSamples                 : {}", acquisitionConfig_.preTriggerSamples);
  spdlog::info("  PostTriggerSamples                : {}", acquisitionConfig_.postTriggerSamples);

  const std::string triggerModeListStr = [&]() {
    std::vector<size_t> triggerModeInt{};
    for (const auto mode : acquisitionConfig_.triggerModes) {
      triggerModeInt.push_back(static_cast<size_t>(mode));
    }
    return stringutil::join(triggerModeInt, ", ");
  }();
  spdlog::info("  TriggerModes                      : [{}]", triggerModeListStr);

  spdlog::info("  SamplesInEventPacket              : {}", acquisitionConfig_.samplesInEventPacket);
  spdlog::info("  DownSamplingFactorForSavedWaveform: {}", acquisitionConfig_.downSamplingFactorForSavedWaveform);
  spdlog::info("  ChannelEnable                     : [{}]", stringutil::join(acquisitionConfig_.channelEnable, ", "));
  spdlog::info("  TriggerThresholds                 : [{}]",
               stringutil::join(acquisitionConfig_.triggerThresholds, ", "));
  spdlog::info("  TriggerCloseThresholds            : [{}]",
               stringutil::join(acquisitionConfig_.triggerCloseThresholds, ", "));
}

void GROWTH_FY2015_ADC::programDigitizer() {
  spdlog::info("Programming the digitizer");
  try {
    // record length
    setNumberOfSamples(acquisitionConfig_.preTriggerSamples + acquisitionConfig_.postTriggerSamples);
    setNumberOfSamplesInEventPacket(acquisitionConfig_.samplesInEventPacket);
    for (size_t ch = 0; ch < growth_fpga::NumberOfChannels; ch++) {
      // pre-trigger (delay)
      setDepthOfDelay(ch, acquisitionConfig_.preTriggerSamples);

      // trigger mode
      setTriggerMode(ch, acquisitionConfig_.triggerModes.at(ch));

      // threshold
      setStartingThreshold(ch, acquisitionConfig_.triggerThresholds[ch]);
      setClosingThreshold(ch, acquisitionConfig_.triggerCloseThresholds[ch]);

      // adc clock 50MHz
      setAdcClock(growth_fpga::ADCClockFrequency::ADCClock50MHz);

      // turn on ADC
      turnOnADCPower(ch);
    }
    spdlog::info("Device configuration is done");
  } catch (const std::exception& e) {
    spdlog::error("Device configuration has failed ({})", e.what());
    throw;
  } catch (const RMAPInitiatorException& e) {
    spdlog::error("Communication failed ({})", e.toString());
    throw;
  } catch (...) {
    spdlog::error("Device configuration has failed");
    throw;
  }
}
