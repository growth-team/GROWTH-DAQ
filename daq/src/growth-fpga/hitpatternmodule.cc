#include "growth-fpga/hitpatternmodule.hh"

#include <array>
#include <memory>
#include <cassert>

HitPatternSettings::HitPatternSettings(u16 _lowerThreshold, u16 _upperThreshold, u16 _width, u16 _delay)
    : lowerThreshold(_lowerThreshold), upperThreshold(_upperThreshold), width(_width), delay(_delay) {}
bool HitPatternSettings::operator==(const HitPatternSettings& right) const {
  return (lowerThreshold == right.lowerThreshold) && (upperThreshold == right.upperThreshold) &&
         (width == right.width) && (delay == right.delay);
}
bool HitPatternSettings::operator!=(const HitPatternSettings& right) const { return !(*this == right); }

HitPatternModule::HitPatternModule(RMAPInitiatorPtr rmapInitiator, std::shared_ptr<RMAPTargetNode> rmapTargetNode)
    : RegisterAccessInterface(std::move(rmapInitiator), std::move(rmapTargetNode)) {}

void HitPatternModule::writeSettings(size_t ch, const HitPatternSettings& hitPatternSettings) {
  assert(ch < NUM_CHANNELS);
  assert(hitPatternSettings.lowerThreshold <= MAX_THRESHOLD);
  assert(hitPatternSettings.upperThreshold <= MAX_THRESHOLD);
  assert(hitPatternSettings.lowerThreshold < hitPatternSettings.upperThreshold);
  assert(hitPatternSettings.width <= MAX_WIDTH);
  assert(hitPatternSettings.delay <= MAX_DELAY);

  // Write registers
  write(ADDRESS_THRESHOLD_LOW.at(ch), hitPatternSettings.lowerThreshold);
  write(ADDRESS_THRESHOLD_HIGH.at(ch), hitPatternSettings.upperThreshold);
  write(ADDRESS_HIT_PATTERN_WIDTH.at(ch), hitPatternSettings.width);
  write(ADDRESS_HIT_PATTERN_DELAY.at(ch), hitPatternSettings.delay);

  // Read back then compare
  if (readSettings(ch) != hitPatternSettings) {
    throw std::runtime_error("Failed to set the hit-pattern registers");
  }
}

HitPatternSettings HitPatternModule::readSettings(size_t ch) const {
  const u16 lowerThreshold = read16(ADDRESS_THRESHOLD_LOW.at(ch));
  const u16 upperThreshold = read16(ADDRESS_THRESHOLD_HIGH.at(ch));
  const u16 width = read16(ADDRESS_HIT_PATTERN_WIDTH.at(ch));
  const u16 delay = read16(ADDRESS_HIT_PATTERN_DELAY.at(ch));
  return {lowerThreshold, upperThreshold, width, delay};
}

void HitPatternModule::setOrSwitch(u16 value) {
  write(ADDRESS_HIT_PATTERN_OR_SWITCH, value);
  if (readOrSwitch() != value) {
    throw std::runtime_error("Failed to set the 'OR' switch register");
  }
}

u16 HitPatternModule::readOrSwitch() const { return read16(ADDRESS_HIT_PATTERN_OR_SWITCH); }
