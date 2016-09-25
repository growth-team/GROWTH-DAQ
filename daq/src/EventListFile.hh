/*
 * EventListFile.hh
 *
 *  Created on: Sep 29, 2015
 *      Author: yuasa
 */

#ifndef EVENTLISTFILE_HH_
#define EVENTLISTFILE_HH_

#include "CxxUtilities/CxxUtilities.hh"
#include "GROWTH_FY2015_ADCModules/Types.hh"

/** Represents an event list file.
 */
class EventListFile {
protected:
	std::string fileName;

public:
	EventListFile(std::string fileName) :
			fileName(fileName) {
	}

public:
	virtual ~EventListFile() {

	}

public:
	virtual void fillEvents(std::vector<GROWTH_FY2015_ADC_Type::Event*>& events) =0;

public:
	virtual size_t getEntries() =0;

public:
	virtual void close() =0;

public:
	virtual void fillGPSTime(uint8_t* gpsTimeRegisterBuffer) =0;
};

#endif /* EVENTLISTFILE_HH_ */
