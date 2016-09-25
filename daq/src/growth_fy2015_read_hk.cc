/*
 * growth_fy2015_readADC.cc
 *
 *  Created on: Nov 3, 2015
 *      Author: yuasa
 */

#include "ADCDAC.hh"
#include "CxxUtilities/CxxUtilities.hh"
#include <iostream>

int main(int argc, char* argv[]) {
	using namespace std;
	if (argc < 2) {
		cerr << "Provide measurement duration in secodnds." << endl;
		exit(-1);
	}

	CxxUtilities::Condition condition;
	static const double WaitDuration = 10000; //ms = 10s
	bool loop = true;
	size_t elapsedTime = 0;
	size_t duration = atoi(argv[1]);
	ADCDAC adcdac;
	while (loop) {
		ADCData adcData = adcdac.getADCData();
		uint32_t unixTime = CxxUtilities::Time::getUNIXTimeAsUInt32();
		cout << unixTime << " " << adcData.toString() << endl;
		condition.wait(WaitDuration);
		elapsedTime++;
		if (elapsedTime >= duration) {
			loop = false;
		}
	}
}

