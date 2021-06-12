#include "growth-fpga/channelmanager.hh"

#include <bitset>

ChannelManager::ChannelManager(std::shared_ptr<RMAPInitiator> rmapInitiator,
                               std::shared_ptr<RMAPTargetNode> rmapTargetNode)
    : RegisterAccessInterface(rmapInitiator, rmapTargetNode),
      startStopSemaphore_(rmapInitiator, rmapTargetNode, AddressOf_StartStopSemaphoreRegister) {}

void ChannelManager::startAcquisition(const std::vector<bool>& channelsToBeStarted) {
  // Prepare register value for StartStopRegister
  const u16 value = [&]() {
    std::bitset<growth_fpga::NumberOfChannels> value{0};
    size_t bitIndex{0};
    for (const auto enabled : channelsToBeStarted) {
      if (enabled) {
        value.set(bitIndex);
      }
      bitIndex++;
    }
    return value.to_ulong();
  }();
  // write the StartStopRegister (semaphore is needed)
  SemaphoreLock lock(startStopSemaphore_);
  write(AddressOf_StartStopRegister, value);
  spdlog::debug("ChannelManager::startAcquisition() readback = {:08b}", read16(AddressOf_StartStopRegister));
}

bool ChannelManager::isAcquisitionCompleted() const {
  const u16 value = read16(AddressOf_StartStopRegister);
  return value == 0x0000;
}

bool ChannelManager::isAcquisitionCompleted(size_t chNumber) const {
  const u16 value = read16(AddressOf_StartStopRegister);
  if (chNumber < growth_fpga::NumberOfChannels) {
    const std::bitset<16> bits(value);
    return bits[chNumber] == 0;
  } else {
    // if chNumber is out of the allowed range
    return false;
  }
}

void ChannelManager::stopAcquisition() {
  constexpr u16 STOP = 0x00;
  SemaphoreLock lock(startStopSemaphore_);
  write(AddressOf_StartStopRegister, STOP);
  spdlog::debug("ChannelManager::stopAcquisition() readback = {:08b}", read16(AddressOf_StartStopRegister));
}

void ChannelManager::setPresetMode(growth_fpga::PresetMode presetMode) {
  write(AddressOf_PresetModeRegister, static_cast<u16>(presetMode));
}

void ChannelManager::setAdcClock(growth_fpga::ADCClockFrequency adcClockFrequency) {
  write(AddressOf_ADCClock_Register, static_cast<u16>(adcClockFrequency));
}

void ChannelManager::setPresetLivetime(u32 livetimeIn10msUnit) {
  write(AddressOf_PresetLivetimeRegisterL, livetimeIn10msUnit);
}

f64 ChannelManager::getRealtime() const { return static_cast<f64>(read48(AddressOf_RealtimeRegisterL)); }

void ChannelManager::reset() {
  constexpr u16 RESET = 0x01;
  write(AddressOf_ResetRegister, RESET);
}

void ChannelManager::setPresetnEvents(u32 nEvents) { write(AddressOf_PresetLivetimeRegisterL, nEvents); }

void ChannelManager::setadcClockCounter(u16 adcClockCounter) { write(AddressOf_ADCClock_Register, adcClockCounter); }
