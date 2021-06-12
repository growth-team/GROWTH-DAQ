#include "growth-fpga/channelmodule.hh"

ChannelModule::ChannelModule(RMAPInitiatorPtr rmapInitiator, std::shared_ptr<RMAPTargetNode> rmapTargetNode,
                             u8 chNumber)
    : RegisterAccessInterface(std::move(rmapInitiator), rmapTargetNode), chNumber_(chNumber) {
  const u32 BA = InitialAddressOf_ChModule_0 + chNumber * AddressOffsetBetweenChannels;
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

void ChannelModule::setTriggerMode(growth_fpga::TriggerMode triggerMode) {
  write(AddressOf_TriggerModeRegister, static_cast<u16>(triggerMode));
}

u32 ChannelModule::getTriggerMode() const { return read16(AddressOf_TriggerModeRegister); }

void ChannelModule::sendCPUTrigger() {
  constexpr u16 TRIG_FIRE = 0xFFFF;
  write(AddressOf_CPUTriggerRegister, TRIG_FIRE);
}

void ChannelModule::setNumberOfSamples(u16 nSamples) { write(AddressOf_NumberOfSamplesRegister, nSamples); }

void ChannelModule::setStartingThreshold(u16 threshold) { write(AddressOf_ThresholdStartingRegister, threshold); }

void ChannelModule::setClosingThreshold(u16 threshold) { write(AddressOf_ThresholdClosingRegister, threshold); }

void ChannelModule::turnADCPower(bool trueifon) {
  constexpr u16 POWER_DOWN = 0xFFFF;
  constexpr u16 POWER_UP = 0x0000;
  write(AddressOf_AdcPowerDownModeRegister, trueifon ? POWER_UP : POWER_DOWN);
}

void ChannelModule::setDepthOfDelay(u16 depthOfDelay) { write(AddressOf_DepthOfDelayRegister, depthOfDelay); }

u32 ChannelModule::getLivetime() const { return read32(AddressOf_LivetimeRegisterL); }

u16 ChannelModule::getCurrentADCValue() const { return read16(AddressOf_CurrentAdcDataRegister); }

size_t ChannelModule::getTriggerCount() const { return read32(AddressOf_TriggerCountRegisterL); }

std::string ChannelModule::getStatus() const {
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
