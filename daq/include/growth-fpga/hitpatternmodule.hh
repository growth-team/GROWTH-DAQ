#ifndef GROWTH_FPGA_HitPatternModule_HH_
#define GROWTH_FPGA_HitPatternModule_HH_

#include "types.h"
#include "growth-fpga/registeraccessinterface.hh"

class HitPatternSettings {
 public:
  HitPatternSettings(u16 _lowerThreshold, u16 _upperThreshold, u16 _width, u16 _delay);
  bool operator==(const HitPatternSettings& right) const;
  bool operator!=(const HitPatternSettings& right) const;
  u16 lowerThreshold{};
  u16 upperThreshold{};
  u16 width{};
  u16 delay{};
};

/// Aggregates hit-pattern settings.
class HitPatternModule : public RegisterAccessInterface {
 public:
  HitPatternModule(RMAPInitiatorPtr rmapInitiator, std::shared_ptr<RMAPTargetNode> rmapTargetNode);
  ~HitPatternModule() override = default;

  void writeSettings(size_t ch, const HitPatternSettings& hitPatternSettings);
  HitPatternSettings readSettings(size_t ch) const;
  void setOrSwitch(u16 value);
  u16 readOrSwitch() const;

 private:
  static constexpr size_t NUM_CHANNELS = 4;
  static constexpr u32 BASE_ADDRESS = 0x01014000;
  static constexpr std::array<u32, NUM_CHANNELS> ADDRESS_THRESHOLD_LOW = {
      BASE_ADDRESS + 0x02,
      BASE_ADDRESS + 0x06,
      BASE_ADDRESS + 0x0a,
      BASE_ADDRESS + 0x0e,
  };
  static constexpr std::array<u32, NUM_CHANNELS> ADDRESS_THRESHOLD_HIGH = {
      BASE_ADDRESS + 0x00,
      BASE_ADDRESS + 0x04,
      BASE_ADDRESS + 0x08,
      BASE_ADDRESS + 0x0c,
  };
  static constexpr std::array<u32, NUM_CHANNELS> ADDRESS_HIT_PATTERN_WIDTH = {
      BASE_ADDRESS + 0x10,
      BASE_ADDRESS + 0x12,
      BASE_ADDRESS + 0x14,
      BASE_ADDRESS + 0x16,
  };
  static constexpr std::array<u32, NUM_CHANNELS> ADDRESS_HIT_PATTERN_DELAY = {
      BASE_ADDRESS + 0x18,
      BASE_ADDRESS + 0x1a,
      BASE_ADDRESS + 0x1c,
      BASE_ADDRESS + 0x1e,
  };
  static constexpr u32 ADDRESS_HIT_PATTERN_OR_SWITCH = BASE_ADDRESS + 0x20;

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
