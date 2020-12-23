#ifndef GROWTH_FPGA_HitPatternModule_HH_
#define GROWTH_FPGA_HitPatternModule_HH_

#include "types.h"
#include "growth-fpga/registeraccessinterface.hh"

#include <cassert>

class RMAPHandlerUART;

class HitPatternSettings {
 public:
  HitPatternSettings(u16 _lowerThreshold, u16 _upperThreshold, u16 _width, u16 _delay)
      : lowerThreshold(_lowerThreshold), upperThreshold(_upperThreshold), width(_width), delay(_delay) {}
  bool operator==(const HitPatternSettings& right) const {
    return (lowerThreshold == right.lowerThreshold) && (upperThreshold == right.upperThreshold) &&
           (width == right.width) && (delay == right.delay);
  }
  bool operator!=(const HitPatternSettings& right) const { return !(*this == right); }
  u16 lowerThreshold{};
  u16 upperThreshold{};
  u16 width{};
  u16 delay{};
};

/// Aggregates hit-pattern settings.
class HitPatternModule : public RegisterAccessInterface {
 public:
  /** Constructor.
   * @param[in] rmapHandler RMAPHandlerUART
   * @param[in] adcRMAPTargetNode RMAPTargetNode that corresponds to the ADC board
   */
  HitPatternModule(std::shared_ptr<RMAPHandlerUART> rmapHandler, std::shared_ptr<RMAPTargetNode> rmapTargetNode)
      : RegisterAccessInterface(rmapHandler, rmapTargetNode) {}
  ~HitPatternModule() override = default;

  void writeSettings(size_t ch, const HitPatternSettings& hitPatternSettings) {
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

  HitPatternSettings readSettings(size_t ch) const {
    const u16 lowerThreshold = read16(ADDRESS_THRESHOLD_LOW.at(ch));
    const u16 upperThreshold = read16(ADDRESS_THRESHOLD_HIGH.at(ch));
    const u16 width = read16(ADDRESS_HIT_PATTERN_WIDTH.at(ch));
    const u16 delay = read16(ADDRESS_HIT_PATTERN_DELAY.at(ch));
    return {lowerThreshold, upperThreshold, width, delay};
  }

  void setOrSwitch(u16 value) {
    write(ADDRESS_HIT_PATTERN_OR_SWITCH, value);
    if (readOrSwitch() != value) {
      throw std::runtime_error("Failed to set the 'OR' switch register");
    }
  }

  u16 readOrSwitch() const { return read16(ADDRESS_HIT_PATTERN_OR_SWITCH); }

 private:
  static constexpr size_t NUM_CHANNELS = 4;
  static constexpr u32 BASE_HIT_PATTERN_REGISTERS = 0x01014000;
  static constexpr std::array<u32, NUM_CHANNELS> ADDRESS_THRESHOLD_LOW = {
      BASE_HIT_PATTERN_REGISTERS + 0x02,
      BASE_HIT_PATTERN_REGISTERS + 0x06,
      BASE_HIT_PATTERN_REGISTERS + 0x0a,
      BASE_HIT_PATTERN_REGISTERS + 0x0e,
  };
  static constexpr std::array<u32, NUM_CHANNELS> ADDRESS_THRESHOLD_HIGH = {
      BASE_HIT_PATTERN_REGISTERS + 0x00,
      BASE_HIT_PATTERN_REGISTERS + 0x04,
      BASE_HIT_PATTERN_REGISTERS + 0x08,
      BASE_HIT_PATTERN_REGISTERS + 0x0c,
  };
  static constexpr std::array<u32, NUM_CHANNELS> ADDRESS_HIT_PATTERN_WIDTH = {
      BASE_HIT_PATTERN_REGISTERS + 0x10,
      BASE_HIT_PATTERN_REGISTERS + 0x12,
      BASE_HIT_PATTERN_REGISTERS + 0x14,
      BASE_HIT_PATTERN_REGISTERS + 0x16,
  };
  static constexpr std::array<u32, NUM_CHANNELS> ADDRESS_HIT_PATTERN_DELAY = {
      BASE_HIT_PATTERN_REGISTERS + 0x18,
      BASE_HIT_PATTERN_REGISTERS + 0x1a,
      BASE_HIT_PATTERN_REGISTERS + 0x1c,
      BASE_HIT_PATTERN_REGISTERS + 0x1e,
  };
  static constexpr u32 ADDRESS_HIT_PATTERN_OR_SWITCH = BASE_HIT_PATTERN_REGISTERS + 0x20;

  static constexpr u16 MAX_THRESHOLD = 4095;
  static constexpr u16 MAX_WIDTH = 1023;
  static constexpr u16 MAX_DELAY = 1023;

  // Threshold: 12 bits
  // Hit-pattern width: 10 bits
  // Hit-pattern delay: 10 bits
  // Hit-pattern OR switch:
  //   Bit 15   12  11     8   7     4   3     0
  //        Line3     Line2     Line1     Line0
  //   Ch  3 2 1 0   3 2 1 0   3 2 1 0   3 2 1 0
};

#endif
