#include "growth-fpga/channelmodule.hh"

ChannelModule::ChannelModule(RMAPInitiatorPtr rmapInitiator, std::shared_ptr<RMAPTargetNode> rmapTargetNode,
                             u8 chNumber)
    : RegisterAccessInterface(std::move(rmapInitiator), rmapTargetNode),
      chNumber_(chNumber),
      BASE_ADDRESS(INITIAL_ADDRESS_CH_MODULE_CH0 + chNumber * ADDRESS_OFFSET_PER_CHANNEL),
      ADDRESS_TRIGGER_MODE_REGISTER(BASE_ADDRESS + 0x0002),
      ADDRESS_NUMBER_OF_SAMPLES_REGISTER(BASE_ADDRESS + 0x0004),
      ADDRESS_THRESHOLD_STARTING_REGISTER(BASE_ADDRESS + 0x0006),
      ADDRESS_THRESHOLD_CLOSING_REGISTER(BASE_ADDRESS + 0x0008),
      ADDRESS_ADC_POWER_DOWN_MODE_REGISTER(BASE_ADDRESS + 0x000a),
      ADDRESS_DEPTH_OF_DELAY_REGISTER(BASE_ADDRESS + 0x000c),
      ADDRESS_LIVETIME_REGISTER_L(BASE_ADDRESS + 0x000e),
      ADDRESS_LIVETIME_REGISTER_H(BASE_ADDRESS + 0x0010),
      ADDRESS_CURRENT_ADC_DATA_REGISTER(BASE_ADDRESS + 0x0012),
      ADDRESS_CPU_TRIGGER_REGISTER(BASE_ADDRESS + 0x0014),
      ADDRESS_TRIGGER_COUNT_REGISTER_L(BASE_ADDRESS + 0x0016),
      ADDRESS_TRIGGER_COUNT_REGISTER_H(BASE_ADDRESS + 0x0018),
      ADDRESS_STATUS1_REGISTER(BASE_ADDRESS + 0x0030) {}

void ChannelModule::setTriggerMode(growth_fpga::TriggerMode triggerMode) {
  write(ADDRESS_TRIGGER_MODE_REGISTER, static_cast<u16>(triggerMode));
}

u32 ChannelModule::getTriggerMode() const { return read16(ADDRESS_TRIGGER_MODE_REGISTER); }

void ChannelModule::sendCPUTrigger() {
  constexpr u16 TRIG_FIRE = 0xFFFF;
  write(ADDRESS_CPU_TRIGGER_REGISTER, TRIG_FIRE);
}

void ChannelModule::setNumberOfSamples(u16 nSamples) { write(ADDRESS_NUMBER_OF_SAMPLES_REGISTER, nSamples); }

void ChannelModule::setStartingThreshold(u16 threshold) { write(ADDRESS_THRESHOLD_STARTING_REGISTER, threshold); }

void ChannelModule::setClosingThreshold(u16 threshold) { write(ADDRESS_THRESHOLD_CLOSING_REGISTER, threshold); }

void ChannelModule::turnADCPower(bool trueifon) {
  constexpr u16 POWER_DOWN = 0xFFFF;
  constexpr u16 POWER_UP = 0x0000;
  write(ADDRESS_ADC_POWER_DOWN_MODE_REGISTER, trueifon ? POWER_UP : POWER_DOWN);
}

void ChannelModule::setDepthOfDelay(u16 depthOfDelay) { write(ADDRESS_DEPTH_OF_DELAY_REGISTER, depthOfDelay); }

u32 ChannelModule::getLivetime() const { return read32(ADDRESS_LIVETIME_REGISTER_L); }

u16 ChannelModule::getCurrentADCValue() const { return read16(ADDRESS_CURRENT_ADC_DATA_REGISTER); }

size_t ChannelModule::getTriggerCount() const { return read32(ADDRESS_TRIGGER_COUNT_REGISTER_L); }

std::string ChannelModule::getStatus() const {
  const auto statusRegister = read16(ADDRESS_STATUS1_REGISTER);
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
