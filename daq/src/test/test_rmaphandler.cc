/*
 * test_rmaphandler.cc
 *
 *  Created on: Jun 10, 2015
 *      Author: yuasa
 */
#include <algorithm>
#include "GROWTH_FY2015_ADC.hh"
// ChannelManager
static const uint32_t InitialAddressOf_ChMgr               = 0x01010000;
static const uint32_t ChMgrBA                              = InitialAddressOf_ChMgr;  // Base Address of ChMgr
static const uint32_t AddressOf_StartStopRegister          = ChMgrBA + 0x0002;
static const uint32_t AddressOf_StartStopSemaphoreRegister = ChMgrBA + 0x0004;
static const uint32_t AddressOf_PresetModeRegister         = ChMgrBA + 0x0006;
static const uint32_t AddressOf_PresetLivetimeRegisterL    = ChMgrBA + 0x0008;
static const uint32_t AddressOf_PresetLivetimeRegisterH    = ChMgrBA + 0x000a;
static const uint32_t AddressOf_RealtimeRegisterL          = ChMgrBA + 0x000c;
static const uint32_t AddressOf_RealtimeRegisterM          = ChMgrBA + 0x000e;
static const uint32_t AddressOf_RealtimeRegisterH          = ChMgrBA + 0x0010;
static const uint32_t AddressOf_ResetRegister              = ChMgrBA + 0x0012;
static const uint32_t AddressOf_ADCClock_Register          = ChMgrBA + 0x0014;
static const uint32_t AddressOf_PresetnEventsRegisterL     = ChMgrBA + 0x0020;
static const uint32_t AddressOf_PresetnEventsRegisterH     = ChMgrBA + 0x0022;

static const uint32_t AddressOf_EventFIFO_DataCountRegister = 0x20000000;
static const uint32_t AddressOf_GPSTimeRegister             = 0x20000002;
static const size_t GPSRegisterLengthInBytes                = 20;  // bytes

static const uint32_t AddressOf_CurrentADCRegister_Ch1 = 0x01011112;

// ConsumerManager
static const uint32_t InitialAddressOf_ConsumerMgr                        = 0x01010000;
static const uint32_t ConsumerMgrBA                                       = InitialAddressOf_ConsumerMgr;
static const uint32_t AddressOf_EventOutputDisableRegister                = ConsumerMgrBA + 0x0100;
static const uint32_t AddressOf_GateSize_FastGate_Register                = ConsumerMgrBA + 0x010e;
static const uint32_t AddressOf_GateSize_SlowGate_Register                = ConsumerMgrBA + 0x0110;
static const uint32_t AddressOf_NumberOf_BaselineSample_Register          = ConsumerMgrBA + 0x0112;
static const uint32_t AddressOf_ConsumerMgr_ResetRegister                 = ConsumerMgrBA + 0x0114;
static const uint32_t AddressOf_EventPacket_NumberOfWaveform_Register     = ConsumerMgrBA + 0x0116;
static const uint32_t AddressOf_EventPacket_WaveformDownSampling_Register = ConsumerMgrBA + 0xFFFF;

int main(int argc, char* argv[]) {
  using namespace std;
  if (argc < 2) {
    cout << "Provide UART device name (e.g. /dev/tty.usb-aaa-bbb)." << endl;
    exit(-1);
  }
  std::string deviceName(argv[1]);

  RMAPTargetNode target;
  target.setID("ADCBox");
  target.setInitiatorLogicalAddress(0xFE);
  target.setTargetLogicalAddress(0xFE);
  target.setDefaultKey(0x00);
  target.setReplyAddress({});

  RMAPHandlerUART* rmapHandler = new RMAPHandlerUART(deviceName, {&target});
  if (rmapHandler->connectoToSpaceWireToGigabitEther() == false) { exit(-1); }
  printf("EventFIFO data count = %d\n", rmapHandler->getRegister(AddressOf_EventFIFO_DataCountRegister));
  printf("ADC Ch.1 = %d\n", rmapHandler->getRegister(AddressOf_CurrentADCRegister_Ch1));

  ChannelModule* channelModule              = new ChannelModule(rmapHandler, &target, 1);
  ChannelManager* channelManager            = new ChannelManager(rmapHandler, &target);
  ConsumerManagerEventFIFO* consumerManager = new ConsumerManagerEventFIFO(rmapHandler, &target);

  cout << channelModule->getStatus() << endl;

  consumerManager->setEventPacket_NumberOfWaveform(1000);
  consumerManager->enableEventDataOutput();
  channelModule->setTriggerMode(SpaceFibreADC::TriggerMode::StartThreshold_NSamples_CloseThreshold);
  channelModule->setStartingThreshold(580);
  channelModule->setClosingThreshold(580);
  channelModule->setNumberOfSamples(1000);

  cout << "Turn on ADC" << endl;
  channelModule->turnADCPower(true);

  printf("Semaphore = %d\n", rmapHandler->getRegister(AddressOf_StartStopSemaphoreRegister));
  cout << "Request" << endl;
  rmapHandler->setRegister(AddressOf_StartStopSemaphoreRegister, 0xFFFF);
  printf("Semaphore = %d\n", rmapHandler->getRegister(AddressOf_StartStopSemaphoreRegister));
  cout << "Start" << endl;
  rmapHandler->setRegister(AddressOf_StartStopRegister, 0x0F);
  printf("StartStop = %x\n", rmapHandler->getRegister(AddressOf_StartStopRegister));
  cout << "Release" << endl;
  rmapHandler->setRegister(AddressOf_StartStopSemaphoreRegister, 0x0000);
  printf("Semaphore = %d\n", rmapHandler->getRegister(AddressOf_StartStopSemaphoreRegister));

  printf("StartStop = %x\n", rmapHandler->getRegister(AddressOf_StartStopRegister));
  printf("Realtime = %d\n", rmapHandler->getRegister(AddressOf_RealtimeRegisterL));

  printf("Livetime Ch.1 = %d\n", channelModule->getLivetime());
  printf("ADC Ch.1 = %d\n", channelModule->getCurrentADCValue());

  cout << channelModule->getStatus() << endl;

  CxxUtilities::Condition c;
  c.wait(1000);

  size_t eventFIFODataCount;

  printf("Trigger count = %d\n", channelModule->getTriggerCount());
  printf("ADC Ch.1 = %d\n", channelModule->getCurrentADCValue());

  const size_t bufferMax = 2000;
  uint8_t* buffer        = new uint8_t[bufferMax];

  cout << "Reading GPS Time register" << endl;
  uint8_t gpsTime[GPSRegisterLengthInBytes];
  rmapHandler->read(&target, AddressOf_GPSTimeRegister, GPSRegisterLengthInBytes, gpsTime);
  for (size_t i = 0; i < GPSRegisterLengthInBytes - 6; i++) { cout << gpsTime[i]; }
  cout << dec << endl;
  c.wait(2000);
  rmapHandler->read(&target, AddressOf_GPSTimeRegister, GPSRegisterLengthInBytes, gpsTime);
  for (size_t i = 0; i < GPSRegisterLengthInBytes - 6; i++) { cout << gpsTime[i]; }
  cout << dec << endl;

  exit(-1);

  do {
    eventFIFODataCount = rmapHandler->getRegister(AddressOf_EventFIFO_DataCountRegister);
    printf("EventFIFO data count = %d\n", eventFIFODataCount);

    size_t readSize = eventFIFODataCount * 2;
    if (bufferMax < readSize) { readSize = bufferMax; }

    if (readSize != 0) {
      rmapHandler->read(&target, 0x10000000, readSize, buffer);
      for (size_t i = 0; i < readSize; i++) {
        cout << hex << right << setw(2) << setfill('0') << (uint32_t)buffer[i] << " ";
      }
      cout << dec << endl;
    }
  } while (eventFIFODataCount != 0);
  rmapHandler->disconnectSpWGbE();
}
