#ifndef GROWTH_FPGA_TYPES_HH_
#define GROWTH_FPGA_TYPES_HH_

#include "types.h"
#include "growth-fpga/constants.hh"

#include <memory>

namespace growth_fpga {
enum class TriggerMode : u32 {
  StartThreshold_NSamples_AutoClose = 1,       //
  CommonGateIn = 2,                            //
  StartThreshold_NSamples_CloseThreshold = 3,  //
  DeltaAboveDelayedADC_NumberOfSamples = 4,    //
  CPUTrigger = 5,                              //
  TriggerBusSelectedOR = 8,                    //
  TriggerBusSelectedAND = 9
};

enum class PresetMode : u32 {
  NonStop = 0,         //
  Livetime = 1,        //
  NumberOfEvents = 2,  //
};

struct HouseKeepingData {
  u32 realtime;
  u32 livetime[growth_fpga::NumberOfChannels];
  bool acquisitionStarted[growth_fpga::NumberOfChannels];
};

struct Event {
  explicit Event(size_t numWaveformSamples) : waveform(new u16[numWaveformSamples]) {}
  ~Event() { delete[] waveform; }
  u8 ch;
  u64 timeTag;
  u16 triggerCount;
  u16 phaMax;
  u16 phaMaxTime;
  u16 phaMin;
  u16 phaFirst;
  u16 phaLast;
  u16 maxDerivative;
  u16 baseline;
  u16 nSamples;
  u16* waveform;
};

enum class ADCClockFrequency : u16 { ADCClock200MHz = 20000, ADCClock100MHz = 10000, ADCClock50MHz = 5000 };
}  // namespace growth_fpga

using EventPtr = std::unique_ptr<growth_fpga::Event>;
using EventList = std::vector<EventPtr>;
using EventListPtr = std::unique_ptr<EventList>;
using U8Buffer = std::vector<u8>;
using U8BufferPtr = std::unique_ptr<U8Buffer>;

#endif
