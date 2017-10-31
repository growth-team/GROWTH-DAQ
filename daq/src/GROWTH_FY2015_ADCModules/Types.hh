/*
 * Types.hh
 *
 *  Created on: Oct 27, 2014
 *      Author: yuasa
 */

#ifndef GROWTH_FY2015_ADC_TYPES_HH_
#define GROWTH_FY2015_ADC_TYPES_HH_

#include "GROWTH_FY2015_ADCModules/Constants.hh"

namespace GROWTH_FY2015_ADC_Type {
enum class TriggerMode
	: uint32_t {
		StartThreshold_NSamples_AutoClose = 1, //
	CommonGateIn = 2, //
	StartThreshold_NSamples_CloseThreshold = 3, //
	DeltaAboveDelayedADC_NumberOfSamples = 4, //
	CPUTrigger = 5, //
	TriggerBusSelectedOR = 8, //
	TriggerBusSelectedAND = 9
};

enum class PresetMode
	: uint32_t {
		NonStop = 0, //
	Livetime = 1, //
	NumberOfEvents = 2, //
};

struct HouseKeepingData {
	uint32_t realtime;
	uint32_t livetime[SpaceFibreADC::NumberOfChannels];
	bool acquisitionStarted[SpaceFibreADC::NumberOfChannels];
};

struct Event {
	uint8_t ch;
	uint64_t timeTag;
	uint16_t triggerCount;
	uint16_t phaMax;
	uint16_t phaMaxTime;
	uint16_t phaMin;
	uint16_t phaFirst;
	uint16_t phaLast;
	uint16_t maxDerivative;
	uint16_t baseline;
	uint16_t nSamples;
	uint16_t* waveform;
};

enum class ADCClockFrequency
	: uint16_t {
		ADCClock200MHz = 20000, ADCClock100MHz = 10000, ADCClock50MHz = 5000
};
}

#endif /* GROWTH_FY2015_ADC_TYPES_HH_ */
