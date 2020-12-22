/*
 * test_rmaphandler.cc
 *
 *  Created on: Jun 10, 2015
 *      Author: yuasa
 */
#include <algorithm>

#include "GROWTH_FY2015_ADC.hh"
// ChannelManager
static const u32 InitialAddressOf_ChMgr = 0x01010000;
static const u32 ChMgrBA = InitialAddressOf_ChMgr;  // Base Address of ChMgr
static const u32 AddressOf_StartStopRegister = ChMgrBA + 0x0002;
static const u32 AddressOf_StartStopSemaphoreRegister = ChMgrBA + 0x0004;
static const u32 AddressOf_PresetModeRegister = ChMgrBA + 0x0006;
static const u32 AddressOf_PresetLivetimeRegisterL = ChMgrBA + 0x0008;
static const u32 AddressOf_PresetLivetimeRegisterH = ChMgrBA + 0x000a;
static const u32 AddressOf_RealtimeRegisterL = ChMgrBA + 0x000c;
static const u32 AddressOf_RealtimeRegisterM = ChMgrBA + 0x000e;
static const u32 AddressOf_RealtimeRegisterH = ChMgrBA + 0x0010;
static const u32 AddressOf_ResetRegister = ChMgrBA + 0x0012;
static const u32 AddressOf_ADCClock_Register = ChMgrBA + 0x0014;
static const u32 AddressOf_PresetnEventsRegisterL = ChMgrBA + 0x0020;
static const u32 AddressOf_PresetnEventsRegisterH = ChMgrBA + 0x0022;

static const u32 AddressOf_EventFIFO_DataCountRegister = 0x20000000;
static const u32 AddressOf_GPSTimeRegister = 0x20000002;
static const size_t GPSRegisterLengthInBytes = 20;  // bytes

static const u32 AddressOf_CurrentADCRegister_Ch1 = 0x01011112;

// ConsumerManager
static const u32 InitialAddressOf_ConsumerMgr = 0x01010000;
static const u32 ConsumerMgrBA = InitialAddressOf_ConsumerMgr;
static const u32 AddressOf_EventOutputDisableRegister = ConsumerMgrBA + 0x0100;
static const u32 AddressOf_GateSize_FastGate_Register = ConsumerMgrBA + 0x010e;
static const u32 AddressOf_GateSize_SlowGate_Register = ConsumerMgrBA + 0x0110;
static const u32 AddressOf_NumberOf_BaselineSample_Register = ConsumerMgrBA + 0x0112;
static const u32 AddressOf_ConsumerMgr_ResetRegister = ConsumerMgrBA + 0x0114;
static const u32 AddressOf_EventPacket_NumberOfWaveform_Register = ConsumerMgrBA + 0x0116;
static const u32 AddressOf_EventPacket_WaveformDownSampling_Register = ConsumerMgrBA + 0xFFFF;

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
  if (!rmapHandler->connectoToSpaceWireToGigabitEther()) {
    exit(-1);
  }
  printf("EventFIFO data count = %d\n", rmapHandler->getRegister(AddressOf_EventFIFO_DataCountRegister));
  printf("ADC Ch.1 = %d\n", rmapHandler->getRegister(AddressOf_CurrentADCRegister_Ch1));

  ChannelModule* channelModule = new ChannelModule(rmapHandler, &target, 1);
  ChannelManager* channelManager = new ChannelManager(rmapHandler, &target);
  ConsumerManagerEventFIFO* consumerManager = new ConsumerManagerEventFIFO(rmapHandler, &target);

  cout << channelModule->getStatus() << endl;

  consumerManager->setEventPacket_NumberOfWaveform(1000);
  consumerManager->enableEventDataOutput();
  channelModule->setTriggerMode(GROWTH_FY2015_ADC_Type::TriggerMode::StartThreshold_NSamples_CloseThreshold);
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
  u8* buffer = new u8[bufferMax];

  cout << "Reading GPS Time register" << endl;
  u8 gpsTime[GPSRegisterLengthInBytes];
  rmapHandler->read(&target, AddressOf_GPSTimeRegister, GPSRegisterLengthInBytes, gpsTime);
  for (size_t i = 0; i < GPSRegisterLengthInBytes - 6; i++) {
    cout << gpsTime[i];
  }
  cout << dec << endl;
  c.wait(2000);
  rmapHandler->read(&target, AddressOf_GPSTimeRegister, GPSRegisterLengthInBytes, gpsTime);
  for (size_t i = 0; i < GPSRegisterLengthInBytes - 6; i++) {
    cout << gpsTime[i];
  }
  cout << dec << endl;

  exit(-1);

  do {
    eventFIFODataCount = rmapHandler->getRegister(AddressOf_EventFIFO_DataCountRegister);
    printf("EventFIFO data count = %d\n", eventFIFODataCount);

    size_t readSize = eventFIFODataCount * 2;
    if (bufferMax < readSize) {
      readSize = bufferMax;
    }

    if (readSize != 0) {
      rmapHandler->read(&target, 0x10000000, readSize, buffer);
      for (size_t i = 0; i < readSize; i++) {
        cout << hex << right << setw(2) << setfill('0') << (u32)buffer[i] << " ";
      }
      cout << dec << endl;
    }
  } while (eventFIFODataCount != 0);
  rmapHandler->disconnectSpWGbE();
}
