/*
 * growth_fy2015_read_gps.cc
 *
 *  Created on: Nov 6, 2015
 *      Author: yuasa
 */

#include "GROWTH_FY2015_ADC.hh"
#include "EventListFileFITS.hh"

int main(int argc, char* argv[]) {
	using namespace std;
	if (argc < 4) {
		cerr << "Provide UART device name (e.g. /dev/tty.usb-aaa-bbb)." << endl;
		::exit(-1);
	}
	std::string deviceName(argv[1]);

	using namespace std;
	GROWTH_FY2015_ADC* adcBoard;
	CxxUtilities::Condition c;
	adcBoard = new GROWTH_FY2015_ADC(deviceName);

	//---------------------------------------------
	// Read GPS Register
	//---------------------------------------------
	cout << "Reading GPS Register" << endl;
	uint8_t* buffer = adcBoard->getGPSRegisterUInt8();
	long long timeTag = 0;

	for (size_t i = 14; i < GROWTH_FY2015_ADC::LengthOfGPSTimeRegister; i++) {
		timeTag = buffer[i] + (timeTag << 8);
	}
	std::stringstream ss;
	for (size_t i = 0; i < 14; i++) {
		ss << buffer[i];
	}
	cout << "timeTag=" << timeTag << " " << ss.str() << endl;

	//---------------------------------------------
	// Read GPS Data FIFO
	//---------------------------------------------
	cout << "Clearing GPS Data FIFO" << endl;
	adcBoard->clearGPSDataFIFO();
	cout << "Wait for 1.5 sec" << endl;
	c.wait(2500);
	cout << "Read GPS Data FIFO" << endl;
	std::vector<uint8_t> gpsDataFIFOReadData = adcBoard->readGPSDataFIFO();
	cout << "Read result (" << gpsDataFIFOReadData.size() << " bytes):" << endl;
	for (size_t i = 0; i < gpsDataFIFOReadData.size(); i++) {
		cout << (char)gpsDataFIFOReadData[i];
	}
	cout << endl;
	cout << "---------------------------------------------" << endl;
	for (size_t i = 0; i < gpsDataFIFOReadData.size(); i++) {
		cout << hex << right << setw(2) << setfill('0') << (uint32_t)gpsDataFIFOReadData[i] << " ";
	}
	cout << endl;

	return 0;
}

