/*
 * GROWTH_FY2015_ADC.hh
 *
 *  Created on: May 15, 2009
 *      Author: yuasa
 */

#ifndef GROWTH_FY2015_ADC_HH_
#define GROWTH_FY2015_ADC_HH_

/** @mainpage SpaceFibre ADC C++ API
 * # Overview
 * GROWTH FY2015 ADC board carries 4-ch 65-Msps pipeline ADC,
 * USB-UART interface, FPGA, and Raspberry Pi 2.
 * GROWTH_FY2015_ADC.hh and related header files provide API for
 * communicating with the GROWTH FY2015 ADC board using C++.
 *
 * # Structure
 * GROWTH_FY2015_ADC is the top-level class which provides
 * all user-level methods to control and configure the board,
 * to set/start/stop the evnet acquisition mode, and
 * to decode recorded event data and waveform.
 * For available methods,
 * see GROWTH_FY2015_ADC API page.
 *
 * # Typical usage
 * -# Include the following header files.
 *   - SpaceWireRMAPLibrary/SpaceWire.hh
 *   - SpaceWireRMAPLibrary/RMAP.hh
 *   - SpaceWireRMAPLibrary/Boards/SpaceFibreADC.hh
 * -# Instantiate the GROWTH_FY2015_ADC class.
 * -# Do configuration:
 *   - Trigger mode.
 *   - Threshold.
 *   - Waveform record length.
 *   - Preset mode (to automatically stop measurement).
 *     - Livetime preset.
 *     - Event count preset.
 *     - No preset (measurement continues forever unless manually stopped).
 * -# Start measurement.
 * -# Read event data. Loop until measurement completes.
 * -# Stop measurement.
 * -# Close device.
 *
 * # Contact
 * - Takayuki Yuasa
 *   - email: takayuki.yuasa at riken.jp
 *
 * # Example user application
 * @code
 //---------------------------------------------
 // Include SpaceWireRMAPLibrary
 #include "SpaceWireRMAPLibrary/SpaceWire.hh"
 #include "SpaceWireRMAPLibrary/RMAP.hh"

 //---------------------------------------------
 // Include SpaceFibreADC
 #include "SpaceWireRMAPLibrary/Boards/SpaceFibreADC.hh"

 const size_t nSamples = 200;
 const size_t nCPUTrigger=10;

 int main(int argc, char* argv[]){
 using namespace std;

 //---------------------------------------------
 // Construct an instance of GROWTH_FY2015_ADC
 // The default IP address is 192.168.1.100 .
 // If your board has set a specific IP address,
 // modify the following line.
 GROWTH_FY2015_ADC* adc=new GROWTH_FY2015_ADC("192.168.1.100");

 //---------------------------------------------
 // Device open
 cout << "Opening device" << endl;
 adc->openDevice();
 cout << "Opened" << endl;

 //---------------------------------------------
 // Trigger Mode
 size_t ch=0;
 cout << "Setting trigger mode" << endl;

 // Trigger Mode: CPU Trigger
 adc->setTriggerMode(ch, TriggerMode::CPUTrigger);

 // Trigger Mode: StartingThreshold-NSamples-ClosingThreshold
 // uint16_t startingThreshold=2300;
 // uint16_t closingThreshold=2100;
 // adc->setStartingThreshold(ch, startingThreshold);
 // adc->setClosingThreshold(ch, closingThreshold);
 //adc->setTriggerMode(ch, TriggerMode::StartThreshold_NSamples_CloseThreshold);

 //---------------------------------------------
 // Waveform record length
 cout << "Setting number of samples" << endl;
 adc->setNumberOfSamples(200);
 cout << "Setting depth of delay" << endl;
 adc->setDepthOfDelay(ch, 20);
 cout << "Setting ADC power" << endl;
 adc->turnOnADCPower(ch);


 //---------------------------------------------
 // Start data acquisition
 cout << "Starting acquisition" << endl;
 adc->startAcquisition({true,false,false,false});


 //---------------------------------------------
 // Send CPU Trigger
 cout << "Sending CPU Trigger " << dec << nCPUTrigger << " times" << endl;
 for(size_t i=0;i<nCPUTrigger;i++){
 adc->sendCPUTrigger(ch);
 }


 //---------------------------------------------
 // Read event data
 std::vector<GROWTH_FY2015_ADC_Type::Event*> events;

 cout << "Reading events" << endl;
 events=adc->getEvent();
 cout << "nEvents = " << events.size() << endl;

 size_t i=0;
 for(auto event : events){
 cout << dec;
 cout << "=============================================" << endl;
 cout << "Event " << i << endl;
 cout << "---------------------------------------------" << endl;
 cout << "timeTag = " << event->timeTag << endl;
 cout << "triggerCount = " << event->triggerCount << endl;
 cout << "phaMax = " << (uint32_t)event->phaMax << endl;
 cout << "nSamples = " << (uint32_t)event->nSamples << endl;
 cout << "waveform = ";
 for(size_t i=0;i<event->nSamples;i++){
 cout << dec << (uint32_t)event->waveform[i] << " ";
 }
 cout << endl;
 i++;
 adc->freeEvent(event);
 }

 //---------------------------------------------
 // Reads HK data
 SpaceFibreADC::HouseKeepingData hk=adc->getHouseKeepingData();
 cout << "livetime = " << hk.livetime[0] << endl;
 cout << "realtime = " << hk.realtime << endl;
 cout << dec;

 //---------------------------------------------
 // Reads current ADC value (un-comment out the following lines to execute)
 // cout << "Current ADC values:"  << endl;
 // for(size_t i=0;i<10;i++){
 // 	cout << adc->getCurrentADCValue(ch) << endl;
 // }

 //---------------------------------------------
 // Stop data acquisition
 cout << "Stopping acquisition" << endl;
 adc->stopAcquisition();

 //---------------------------------------------
 // Device close
 cout << "Closing device" << endl;
 adc->closeDevice();
 cout << "Closed" << endl;

 }
 * @endcode
 */

#include "GROWTH_FY2015_ADCModules/RMAPHandlerUART.hh"
#include "GROWTH_FY2015_ADCModules/Constants.hh"
#include "GROWTH_FY2015_ADCModules/Types.hh"
#include "GROWTH_FY2015_ADCModules/Debug.hh"
#include "GROWTH_FY2015_ADCModules/SemaphoreRegister.hh"
#include "GROWTH_FY2015_ADCModules/ConsumerManagerEventFIFO.hh"
#include "GROWTH_FY2015_ADCModules/EventDecoder.hh"
#include "GROWTH_FY2015_ADCModules/ChannelModule.hh"
#include "GROWTH_FY2015_ADCModules/ChannelManager.hh"
#include "yaml-cpp/yaml.h"

using GROWTH_FY2015_ADC_Type::TriggerMode;

enum class SpaceFibreADCException {
	InvalidChannelNumber, OpenDeviceFailed, CloseDeviceFailed,
};

/** A class which represents SpaceFibre ADC Board.
 * It controls start/stop of data acquisition.
 * It contains GROWTH_FY2015_ADC::SpaceFibreADC::NumberOfChannels instances of
 * ADCChannel class so that
 * a user can change individual registers in each module.
 */
class GROWTH_FY2015_ADC {
public:
	static const uint32_t AddressOfGPSTimeRegister = 0x20000002;
	static const size_t LengthOfGPSTimeRegister = 20;
	static const uint32_t AddressOfGPSDataFIFOResetRegister = 0x20001000;
	static const uint32_t InitialAddressOfGPSDataFIFO = 0x20001002;
	static const uint32_t FinalAddressOfGPSDataFIFO = 0x20001FFF;

	static const uint32_t AddressOfFPGATypeRegister_L = 0x30000000;
	static const uint32_t AddressOfFPGATypeRegister_H = 0x30000002;
	static const uint32_t AddressOfFPGAVersionRegister_L = 0x30000004;
	static const uint32_t AddressOfFPGAVersionRegister_H = 0x30000006;

public:
	class GROWTH_FY2015_ADCDumpThread: public CxxUtilities::StoppableThread {
	private:
		GROWTH_FY2015_ADC* parent;
		EventDecoder* eventDecoder;
	public:
		GROWTH_FY2015_ADCDumpThread(GROWTH_FY2015_ADC* parent) {
			this->parent = parent;
			this->eventDecoder = parent->getEventDecoder();
		}

	public:
		static constexpr double WaitDurationInMilliSec = 1000.0;

	public:
		void run() {
			CxxUtilities::Condition c;
			using namespace std;
			size_t nReceivedEvents_previous = 0;
			size_t delta = 0;
			size_t nReceivedEvents_latch;
			while (!this->stopped) {
				c.wait(WaitDurationInMilliSec);
				nReceivedEvents_latch = parent->nReceivedEvents;
				delta = nReceivedEvents_latch - nReceivedEvents_previous;
				nReceivedEvents_previous = nReceivedEvents_latch;
				cout << "GROWTH_FY2015_ADC received " << dec << parent->nReceivedEvents << " events (delta=" << dec
						<< delta << ")." << endl;
				cout << "GROWTH_FY2015_ADC_Type::EventDecoder available Event instances = " << dec
						<< this->eventDecoder->getNAllocatedEventInstances() << endl;
			}
		}
	};

public:
	//Clock Frequency
	static constexpr double ClockFrequency = 100; //MHz

	//Clock Interval
	static constexpr double ClockInterval = 10e-9; //s

	//FPGA TimeTag
	static const uint32_t TimeTagResolutionInNanoSec = 10; //ns

	//PHA Min/Max
	static const uint16_t PHAMinimum = 0;
	static const uint16_t PHAMaximum = 1023;

private:
	RMAPHandler* rmapHandler;
	RMAPTargetNode* adcRMAPTargetNode;
	RMAPInitiator* rmapIniaitorForGPSRegisterAccess;
	ChannelManager* channelManager;
	ConsumerManagerEventFIFO* consumerManager;
	ChannelModule* channelModules[SpaceFibreADC::NumberOfChannels];
	EventDecoder* eventDecoder;
	//GROWTH_FY2015_ADCDumpThread* dumpThread;

public:
	size_t nReceivedEvents = 0;

public:
	/** Constructor.
	 * @param deviceName UART-USB device name (e.g. /dev/tty.usb-aaa-bbb)
	 */
	GROWTH_FY2015_ADC(std::string deviceName) {
		using namespace std;

		cout << "#---------------------------------------------" << endl;
		cout << "# GROWTH_FY2015_ADC Constructor" << endl;
		cout << "#---------------------------------------------" << endl;
		//construct RMAPTargetNode instance
		adcRMAPTargetNode = new RMAPTargetNode;
		adcRMAPTargetNode->setID("ADCBox");
		adcRMAPTargetNode->setDefaultKey(0x00);
		adcRMAPTargetNode->setReplyAddress( { });
		adcRMAPTargetNode->setTargetSpaceWireAddress( { });
		adcRMAPTargetNode->setTargetLogicalAddress(0xFE);
		adcRMAPTargetNode->setInitiatorLogicalAddress(0xFE);

		this->rmapHandler = new RMAPHandlerUART(deviceName, { adcRMAPTargetNode });
		bool connected = this->rmapHandler->connectoToSpaceWireToGigabitEther();
		if (!connected) {
			cerr << "SpaceWire interface could not be opened." << endl;
			::exit(-1);
		} else {
			cout << "Connected to SpaceWire interface." << endl;
		}

		//
		rmapIniaitorForGPSRegisterAccess = new RMAPInitiator(this->rmapHandler->getRMAPEngine());

		//create an instance of ChannelManager
		this->channelManager = new ChannelManager(rmapHandler, adcRMAPTargetNode);

		//create an instance of ConsumerManager
		this->consumerManager = new ConsumerManagerEventFIFO(rmapHandler, adcRMAPTargetNode);

		//create instances of ADCChannelRegister
		for (size_t i = 0; i < SpaceFibreADC::NumberOfChannels; i++) {
			this->channelModules[i] = new ChannelModule(rmapHandler, adcRMAPTargetNode, i);
		}

		//event decoder
		this->eventDecoder = new EventDecoder();

		//dump thread
		/*
		 this->dumpThread = new GROWTH_FY2015_ADCDumpThread(this);
		 this->dumpThread->start();
		 */
		cout << "Constructor completes." << endl;
	}

public:
	~GROWTH_FY2015_ADC() {
		using namespace std;
		cout << "GROWTH_FY2015_ADC::~GROWTH_FY2015_ADC(): Deconstructing GROWTH_FY2015_ADC instance." << endl;
		/*
		 cout << "GROWTH_FY2015_ADC::~GROWTH_FY2015_ADC(): Stopping dump thread." << endl;
		 if (this->dumpThread != NULL && this->dumpThread->isInRunMethod()) {
		 this->dumpThread->stop();
		 delete this->dumpThread;
		 }*/

		cout << "GROWTH_FY2015_ADC::~GROWTH_FY2015_ADC(): Deleting RMAP Handler." << endl;
		delete rmapIniaitorForGPSRegisterAccess;
		delete rmapHandler;

		cout << "GROWTH_FY2015_ADC::~GROWTH_FY2015_ADC(): Deleting internal modules." << endl;
		delete this->channelManager;
		delete this->consumerManager;
		for (size_t i = 0; i < SpaceFibreADC::NumberOfChannels; i++) {
			delete this->channelModules[i];
		}
		delete eventDecoder;

		cout << "GROWTH_FY2015_ADC::~GROWTH_FY2015_ADC(): Deleting GPS Data FIFO read buffer." << endl;
		if (gpsDataFIFOReadBuffer != NULL) {
			delete gpsDataFIFOReadBuffer;
		}

		cout << "GROWTH_FY2015_ADC::~GROWTH_FY2015_ADC(): Completed." << endl;
	}

private:
	uint8_t gpsTimeRegister[LengthOfGPSTimeRegister + 1];
	const size_t GPSDataFIFODepthInBytes = 1024;
	uint8_t* gpsDataFIFOReadBuffer = NULL;
	std::vector<uint8_t> gpsDataFIFOData;

public:
	/** Returns FPGA Type as string.
	 */
	uint32_t getFPGAType() {
		uint32_t fpgaType = this->rmapHandler->read32BitRegister(adcRMAPTargetNode, AddressOfFPGATypeRegister_L);
		return fpgaType;
	}

public:
	/** Returns FPGA Version as string.
	 */
	uint32_t getFPGAVersion() {
		using namespace std;
		uint32_t fpgaVersion = this->rmapHandler->read32BitRegister(adcRMAPTargetNode, AddressOfFPGAVersionRegister_L);
		return fpgaVersion;
	}

public:
	/** Returns a GPS Register value.
	 */
	std::string getGPSRegister() {
		using namespace std;
		this->rmapHandler->read(adcRMAPTargetNode, AddressOfGPSTimeRegister, LengthOfGPSTimeRegister, gpsTimeRegister);
		std::stringstream ss;
		for (size_t i = 0; i < LengthOfGPSTimeRegister; i++) {
			ss << gpsTimeRegister[i];
		}
		return ss.str();
	}

public:
	uint8_t* getGPSRegisterUInt8() {
		this->rmapHandler->read(adcRMAPTargetNode, AddressOfGPSTimeRegister, LengthOfGPSTimeRegister, gpsTimeRegister);
		gpsTimeRegister[LengthOfGPSTimeRegister] = 0x00;
		return gpsTimeRegister;
	}

public:
	/** Clears the GPS Data FIFO.
	 *  After clear, new data coming from GPS Receiver will be written to GPS Data FIFO.
	 */
	void clearGPSDataFIFO() {
		uint8_t dummy[2];
		this->rmapHandler->read(adcRMAPTargetNode, AddressOfGPSDataFIFOResetRegister, 2, dummy);
	}

public:
	/** Read GPS Data FIFO.
	 *  After clear, new data coming from GPS Receiver will be written to GPS Data FIFO.
	 */
	std::vector<uint8_t> readGPSDataFIFO() {
		if (gpsDataFIFOData.size() != GPSDataFIFODepthInBytes) {
			gpsDataFIFOData.resize(GPSDataFIFODepthInBytes);
		}
		if (gpsDataFIFOReadBuffer == NULL) {
			gpsDataFIFOReadBuffer = new uint8_t[GPSDataFIFODepthInBytes];
		}
		this->rmapHandler->read(adcRMAPTargetNode, AddressOfGPSDataFIFOResetRegister, GPSDataFIFODepthInBytes,
				gpsDataFIFOReadBuffer);
		memcpy(&(gpsDataFIFOData[0]), gpsDataFIFOReadBuffer, GPSDataFIFODepthInBytes);
		return gpsDataFIFOData;
	}

public:
	/** Returns a corresponding pointer ChannelModule.
	 * @param chNumber Channel number that should be returned.
	 * @return a pointer to an instance of ChannelModule.
	 */
	ChannelModule* getChannelRegister(int chNumber) {
		using namespace std;
		if (Debug::adcbox()) {
			cout << "GROWTH_FY2015_ADC::getChannelRegister(" << chNumber << ")" << endl;
		}
		return channelModules[chNumber];
	}

public:
	/** Returns a pointer to ChannelManager.
	 * @return a pointer to ChannelManager.
	 */
	ChannelManager* getChannelManager() {
		using namespace std;
		if (Debug::adcbox()) {
			cout << "GROWTH_FY2015_ADC::getChannelManager()" << endl;
		}
		return channelManager;
	}

public:
	/** Returns a pointer to ConsumerManager.
	 * @return a pointer to ConsumerManager.
	 */
	ConsumerManagerEventFIFO* getConsumerManager() {
		using namespace std;
		if (Debug::adcbox()) {
			cout << "GROWTH_FY2015_ADC::getConsumerManager()" << endl;
		}
		return consumerManager;
	}

public:
	/** Reset ChannelManager and ConsumerManager modules on
	 * VHDL.
	 */
	void reset() {
		using namespace std;
		if (Debug::adcbox()) {
			cout << "GROWTH_FY2015_ADC::reset()";
		}
		this->channelManager->reset();
		this->consumerManager->reset();
		if (Debug::adcbox()) {
			cout << "done" << endl;
		}
	}

public:
	/** Returns RMAPHandler instance which is
	 * used in this instance.
	 * @return an instance of RMAPHandler used in this instance
	 */
	RMAPHandler* getRMAPHandler() {
		return rmapHandler;
	}

//=============================================
// device-wide functions

public:
	/** Closes the device.
	 */
	void closeDevice() {
		using namespace std;
		try {
			cout << "#stopping event data output" << endl;
			consumerManager->disableEventDataOutput();
			//cout << "#stopping dump thread" << endl;
			//consumerManager->stopDumpThread();
			//if (this->dumpThread != NULL) {
			//	this->dumpThread->stop();
			//}
			cout << "#closing sockets" << endl;
			consumerManager->closeSocket();
			cout << "#disconnecting SpaceWire-to-GigabitEther" << endl;
			rmapHandler->disconnectSpWGbE();
		} catch (...) {
			throw SpaceFibreADCException::CloseDeviceFailed;
		}
	}

//=============================================
private:
	std::vector<GROWTH_FY2015_ADC_Type::Event*> events;

public:
	/** Reads, decodes, and returns event data recorded by the board.
	 * When no event packet is received within a timeout duration,
	 * this method will return empty vector meaning a time out.
	 * Each GROWTH_FY2015_ADC_Type::Event instances pointed by the entities
	 * of the returned vector should be freed after use in the user
	 * application. A freed GROWTH_FY2015_ADC_Type::Event instance will be
	 * reused to represent another event data.
	 * @return a vector containing pointers to decoded event data
	 */
	std::vector<GROWTH_FY2015_ADC_Type::Event*> getEvent() {
		events.clear();
		std::vector<uint8_t> data = consumerManager->getEventData();
		if (data.size() != 0) {
			eventDecoder->decodeEvent(&data);
			events = eventDecoder->getDecodedEvents();
		}
		nReceivedEvents += events.size();
		return events;
	}

public:
	/** Frees an event instance so that buffer area can be reused in the following commands.
	 * @param[in] event event instance to be freed
	 */
	void freeEvent(GROWTH_FY2015_ADC_Type::Event* event) {
		eventDecoder->freeEvent(event);
	}

public:
	/** Frees event instances so that buffer area can be reused in the following commands.
	 * @param[in] events a vector of event instance to be freed
	 */
	void freeEvents(std::vector<GROWTH_FY2015_ADC_Type::Event*>& events) {
		for (auto event : events) {
			eventDecoder->freeEvent(event);
		}
	}

//=============================================
public:
	/** Set trigger mode of the specified channel.
	 * TriggerMode;<br>
	 * StartTh->N_samples = StartThreshold_NSamples_AutoClose<br>
	 * Common Gate In = CommonGateIn<br>
	 * StartTh->N_samples->ClosingTh = StartThreshold_NSamples_CloseThreshold<br>
	 * CPU Trigger = CPUTrigger<br>
	 * Trigger Bus (OR) = TriggerBusSelectedOR<br>
	 * Trigger Bus (AND) = TriggerBusSelectedAND<br>
	 * @param chNumber channel number
	 * @param triggerMode trigger mode (see TriggerMode)
	 */
	void setTriggerMode(size_t chNumber, TriggerMode triggerMode) throw (SpaceFibreADCException) {
		if (chNumber < SpaceFibreADC::NumberOfChannels) {
			channelModules[chNumber]->setTriggerMode(triggerMode);
		} else {
			using namespace std;
			cerr << "Error in setTriggerMode(): invalid channel number " << chNumber << endl;
			throw SpaceFibreADCException::InvalidChannelNumber;
		}
	}

public:
	/** Sets the number of ADC samples recorded for a single trigger.
	 * This method sets the same number to all the channels.
	 * After the trigger, nSamples set via this method will be recorded
	 * in the waveform buffer,
	 * and then analyzed in the Pulse Processing Module to form an event packet.
	 * In this processing, the waveform can be truncated at a specified length,
	 * and see setNumberOfSamplesInEventPacket(uint16_t nSamples) for this
	 * truncating length setting.
	 * @note As of 20141103, the maximum waveform length is 1000.
	 * This value can be increased by modifying the depth of the event FIFO
	 * in the FPGA.
	 * @param nSamples number of adc samples per one waveform data
	 */
	void setNumberOfSamples(uint16_t nSamples) {
		for (size_t i = 0; i < SpaceFibreADC::NumberOfChannels; i++) {
			channelModules[i]->setNumberOfSamples(nSamples);
		}
		setNumberOfSamplesInEventPacket(nSamples);
	}

public:
	/** Sets the number of ADC samples which should be output in the
	 * event packet (this must be smaller than the number of samples
	 * recorded for a trigger, see setNumberOfSamples(uint16_t nSamples) ).
	 * @param[in] nSamples number of ADC samples in the event packet
	 */
	void setNumberOfSamplesInEventPacket(uint16_t nSamples) {
		consumerManager->setEventPacket_NumberOfWaveform(nSamples);
	}

public:
	/** Sets Leading Trigger Threshold.
	 * @param threshold an adc value for leading trigger threshold
	 */
	void setStartingThreshold(size_t chNumber, uint32_t threshold) throw (SpaceFibreADCException) {
		if (chNumber < SpaceFibreADC::NumberOfChannels) {
			channelModules[chNumber]->setStartingThreshold(threshold);
		} else {
			using namespace std;
			cerr << "Error in setStartingThreshold(): invalid channel number " << chNumber << endl;
			throw SpaceFibreADCException::InvalidChannelNumber;
		}
	}

public:
	/** Sets Trailing Trigger Threshold.
	 * @param threshold an adc value for trailing trigger threshold
	 */
	void setClosingThreshold(size_t chNumber, uint32_t threshold) throw (SpaceFibreADCException) {
		if (chNumber < SpaceFibreADC::NumberOfChannels) {
			channelModules[chNumber]->setClosingThreshold(threshold);
		} else {
			using namespace std;
			cerr << "Error in setClosingThreshold(): invalid channel number " << chNumber << endl;
			throw SpaceFibreADCException::InvalidChannelNumber;
		}
	}

public:
	/** Sets TriggerBusMask which is used in TriggerMode==TriggerBus.
	 * @param enabledChannels array of enabled trigger bus channels.
	 */
	void setTriggerBusMask(size_t chNumber, std::vector<size_t> enabledChannels) throw (SpaceFibreADCException) {
		if (chNumber < SpaceFibreADC::NumberOfChannels) {
			channelModules[chNumber]->setTriggerBusMask(enabledChannels);
		} else {
			using namespace std;
			cerr << "Error in setTriggerBusMask(): invalid channel number " << chNumber << endl;
			throw SpaceFibreADCException::InvalidChannelNumber;
		}
	}

public:
	/** Sets depth of delay per trigger. When triggered,
	 * a waveform will be recorded starting from N steps
	 * before of the trigger timing.
	 * @param depthOfDelay number of samples retarded
	 */
	void setDepthOfDelay(size_t chNumber, uint32_t depthOfDelay) throw (SpaceFibreADCException) {
		if (chNumber < SpaceFibreADC::NumberOfChannels) {
			channelModules[chNumber]->setDepthOfDelay(depthOfDelay);
		} else {
			using namespace std;
			cerr << "Error in setDepthOfDelay(): invalid channel number " << chNumber << endl;
			throw SpaceFibreADCException::InvalidChannelNumber;
		}
	}

public:
	/** Gets Livetime.
	 * @return elapsed livetime in 10ms unit
	 */
	uint32_t getLivetime(size_t chNumber) throw (SpaceFibreADCException) {
		if (chNumber < SpaceFibreADC::NumberOfChannels) {
			return channelModules[chNumber]->getLivetime();
		} else {
			using namespace std;
			cerr << "Error in getLivetime(): invalid channel number " << chNumber << endl;
			throw SpaceFibreADCException::InvalidChannelNumber;
		}
	}

public:
	/** Get current ADC value.
	 * @return temporal ADC value
	 */
	uint32_t getCurrentADCValue(size_t chNumber) throw (SpaceFibreADCException) {
		if (chNumber < SpaceFibreADC::NumberOfChannels) {
			return channelModules[chNumber]->getCurrentADCValue();
		} else {
			using namespace std;
			cerr << "Error in getCurrentADCValue(): invalid channel number " << chNumber << endl;
			throw SpaceFibreADCException::InvalidChannelNumber;
		}
	}

public:
	/** Turn on ADC.
	 */
	void turnOnADCPower(size_t chNumber) throw (SpaceFibreADCException) {
		if (chNumber < SpaceFibreADC::NumberOfChannels) {
			channelModules[chNumber]->turnADCPower(true);
		} else {
			using namespace std;
			cerr << "Error in turnOnADCPower(): invalid channel number " << chNumber << endl;
			throw SpaceFibreADCException::InvalidChannelNumber;
		}
	}

public:
	/** Turn off ADC.
	 */
	void turnOffADCPower(size_t chNumber) throw (SpaceFibreADCException) {
		if (chNumber < SpaceFibreADC::NumberOfChannels) {
			channelModules[chNumber]->turnADCPower(false);
		} else {
			using namespace std;
			cerr << "Error in turnOffADCPower(): invalid channel number " << chNumber << endl;
			throw SpaceFibreADCException::InvalidChannelNumber;
		}
	}

//=============================================

public:
	/** Starts data acquisition. Configuration of registers of
	 * individual channels should be completed before invoking this method.
	 * @param channelsToBeStarted vector of bool, true if the channel should be started
	 */
	void startAcquisition(std::vector<bool> channelsToBeStarted) {
		consumerManager->disableEventDataOutput();
		consumerManager->reset(); //reset EventFIFO
		consumerManager->enableEventDataOutput();
		channelManager->startAcquisition(channelsToBeStarted);
	}

public:
	/** Starts data acquisition. Configuration of registers of
	 * individual channels should be completed before invoking this method.
	 * Started channels should have been provided via loadConfigurationFile().
	 * @param channelsToBeStarted vector of bool, true if the channel should be started
	 */
	void startAcquisition() {
		consumerManager->enableEventDataOutput();
		channelManager->startAcquisition(this->ChannelEnable);
	}

public:
	/** Checks if all data acquisition is completed in all channels.
	 * 	return true if data acquisition is stopped.
	 */
	bool isAcquisitionCompleted() {
		return channelManager->isAcquisitionCompleted();
	}

public:
	/** Checks if data acquisition of single channel is completed.
	 * 	return true if data acquisition is stopped.
	 */
	bool isAcquisitionCompleted(size_t chNumber) {
		return channelManager->isAcquisitionCompleted(chNumber);
	}

public:
	/** Stops data acquisition regardless of the preset mode of
	 * the acquisition.
	 */
	void stopAcquisition() {
		channelManager->stopAcquisition();
	}

public:
	/** Sends CPU Trigger to force triggering in the specified channel.
	 * @param[in] chNumber channel to be CPU-triggered
	 */
	void sendCPUTrigger(size_t chNumber) {
		if (chNumber < SpaceFibreADC::NumberOfChannels) {
			channelModules[chNumber]->sendCPUTrigger();
		} else {
			using namespace std;
			cerr << "Error in sendCPUTrigger(): invalid channel number " << chNumber << endl;
			throw SpaceFibreADCException::InvalidChannelNumber;
		}
	}

public:
	/** Sends CPU Trigger to all enabled channels.
	 * @param[in] chNumber channel to be CPU-triggered
	 */
	void sendCPUTrigger() {
		using namespace std;
		for (size_t chNumber = 0; chNumber < SpaceFibreADC::NumberOfChannels; chNumber++) {
			if (this->ChannelEnable[chNumber] == true) { //if enabled
				cout << "CPU Trigger to Channel " << chNumber << endl;
				channelModules[chNumber]->sendCPUTrigger();
			}
		}
	}

public:
	/** Sets measurement preset mode.
	 * Available preset modes are:<br>
	 * PresetMode::Livetime<br>
	 * PresetMode::Number of Event<br>
	 * PresetMode::NonStop (Forever)
	 * @param presetMode preset mode value (see also enum class SpaceFibreADC::PresetMode )
	 */
	void setPresetMode(SpaceFibreADC::PresetMode presetMode) {
		channelManager->setPresetMode(presetMode);
	}

public:
	/** Sets Livetime preset value.
	 * @param livetimeIn10msUnit live time to be set (in a unit of 10ms)
	 */
	void setPresetLivetime(uint32_t livetimeIn10msUnit) {
		channelManager->setPresetLivetime(livetimeIn10msUnit);
	}

public:
	/** Sets PresetnEventsRegister.
	 * @param nEvents number of event to be taken
	 */
	void setPresetnEvents(uint32_t nEvents) {
		channelManager->setPresetnEvents(nEvents);
	}

public:
	/** Get Realtime which is elapsed time since the board power was turned on.
	 * @return elapsed real time in 10ms unit
	 */
	double getRealtime() {
		return channelManager->getRealtime();
	}

//=============================================
public:
	/** Sets ADC Clock.
	 * - ADCClockFrequency::ADCClock200MHz <br>
	 * - ADCClockFrequency::ADCClock100MHz <br>
	 * - ADCClockFrequency::ADCClock50MHz <br>
	 * @param adcClockFrequency enum class ADCClockFrequency
	 */
	void setAdcClock(SpaceFibreADC::ADCClockFrequency adcClockFrequency) {
		channelManager->setAdcClock(adcClockFrequency);
	}

//=============================================
public:
	/** Gets HK data including the real time and the live time which are
	 * counted in the FPGA, and acquisition status (started or stopped).
	 * @retrun HK information contained in a SpaceFibreADC::HouseKeepingData instance
	 */
	SpaceFibreADC::HouseKeepingData getHouseKeepingData() {
		SpaceFibreADC::HouseKeepingData hkData;
		//realtime
		hkData.realtime = channelManager->getRealtime();

		for (size_t i = 0; i < SpaceFibreADC::NumberOfChannels; i++) {
			//livetime
			hkData.livetime[i] = channelModules[i]->getLivetime();
			//acquisition status
			hkData.acquisitionStarted[i] = channelManager->isAcquisitionCompleted(i);
		}

		return hkData;
	}

public:
	/** Returns EventDecoder instance.
	 * @return EventDecoder instance
	 */
	EventDecoder* getEventDecoder() {
		return eventDecoder;
	}

public:
	const size_t nChannels = 4;
	std::string DetectorID;
	size_t PreTriggerSamples = 4;
	size_t PostTriggerSamples = 1000;
	std::vector<enum TriggerMode> TriggerModes { TriggerMode::StartThreshold_NSamples_CloseThreshold,
			TriggerMode::StartThreshold_NSamples_CloseThreshold, TriggerMode::StartThreshold_NSamples_CloseThreshold,
			TriggerMode::StartThreshold_NSamples_CloseThreshold };
	size_t SamplesInEventPacket = 1000;
	size_t DownSamplingFactorForSavedWaveform = 1;
	std::vector<bool> ChannelEnable;
	std::vector<uint16_t> TriggerThresholds;
	std::vector<uint16_t> TriggerCloseThresholds;

public:
	size_t getNSamplesInEventListFile() {
		return (this->SamplesInEventPacket) / this->DownSamplingFactorForSavedWaveform;
	}

public:
	void dumpMustExistKeywords() {
		using namespace std;
		cout << "---------------------------------------------" << endl;
		cout << "The following is a template of a configuration file." << endl;
		cout << "---------------------------------------------" << endl;
		cout << endl;
		cout //
		<< "DetectorID: fy2015a" << endl //
				<< "PreTriggerSamples: 10" << endl //
				<< "PostTriggerSamples: 500" << endl //
				<< "SamplesInEventPacket: 510" << endl //
				<< "DownSamplingFactorForSavedWaveform: 4" << endl //
				<< "ChannelEnable: [true, true, true, true]" << endl //
				<< "TriggerThresholds: [800, 800, 800, 800]" << endl //
				<< "TriggerCloseThresholds: [800, 800, 800, 800]" << endl;
	}

private:
	template<typename T, typename Y>
	std::map<T, Y> parseYAMLMap(YAML::Node node) {
		std::map<T, Y> outputMap;
		for (auto e : node) {
			outputMap.insert( { e.first.as<T>(), e.second.as<Y>() });
		}
		return outputMap;
	}

public:
	void loadConfigurationFile(std::string inputFileName) {
		using namespace std;
		YAML::Node yaml_root = YAML::LoadFile(inputFileName);
		std::vector<std::string> mustExistKeywords = { "DetectorID", "PreTriggerSamples", "PostTriggerSamples",
				"SamplesInEventPacket", "DownSamplingFactorForSavedWaveform", "ChannelEnable", "TriggerThresholds",
				"TriggerCloseThresholds" };

		//---------------------------------------------
		//check keyword existence
		//---------------------------------------------
		for (auto keyword : mustExistKeywords) {
			if (!yaml_root[keyword].IsDefined()) {
				cerr << "Error: " << keyword << " is not defined in the configuration file." << endl;
				dumpMustExistKeywords();
				exit(-1);
			}
		}

		//---------------------------------------------
		//load parameter values from the file
		//---------------------------------------------
		this->DetectorID = yaml_root["DetectorID"].as<std::string>();
		this->PreTriggerSamples = yaml_root["PreTriggerSamples"].as<size_t>();
		this->PostTriggerSamples = yaml_root["PostTriggerSamples"].as<size_t>();
		{
			// Convert integer-type trigger mode to TriggerMode enum type
			const std::vector<size_t> triggerModeInt = yaml_root["TriggerModes"].as<std::vector<size_t>>();
			for (size_t i = 0; i < triggerModeInt.size(); i++) {
				this->TriggerModes[i] = static_cast<enum TriggerMode>(triggerModeInt.at(i));
			}
		}
		this->SamplesInEventPacket = yaml_root["SamplesInEventPacket"].as<size_t>();
		this->DownSamplingFactorForSavedWaveform = yaml_root["DownSamplingFactorForSavedWaveform"].as<size_t>();
		this->ChannelEnable = yaml_root["ChannelEnable"].as<std::vector<bool>>();
		this->TriggerThresholds = yaml_root["TriggerThresholds"].as<std::vector<uint16_t>>();
		this->TriggerCloseThresholds = yaml_root["TriggerCloseThresholds"].as<std::vector<uint16_t>>();

		//---------------------------------------------
		//dump setting
		//---------------------------------------------
		cout << "#---------------------------------------------" << endl;
		cout << "# Configuration" << endl;
		cout << "#---------------------------------------------" << endl;
		cout << "DetectorID                        : " << this->DetectorID << endl;
		cout << "PreTriggerSamples                 : " << this->PreTriggerSamples << endl;
		cout << "PostTriggerSamples                : " << this->PostTriggerSamples << endl;
		{
			cout << "TriggerModes                      : [";
			std::vector<size_t> triggerModeInt { };
			for (const auto mode : this->TriggerModes) {
				triggerModeInt.push_back(static_cast<size_t>(mode));
			}
			cout << CxxUtilities::String::join(triggerModeInt, ", ") << "]" << endl;
		}
		cout << "SamplesInEventPacket              : " << this->SamplesInEventPacket << endl;
		cout << "DownSamplingFactorForSavedWaveform: " << this->DownSamplingFactorForSavedWaveform << endl;
		cout << "ChannelEnable                     : [" << CxxUtilities::String::join(this->ChannelEnable, ", ") << "]"
				<< endl;
		cout << "TriggerThresholds                 : [" << CxxUtilities::String::join(this->TriggerThresholds, ", ")
				<< "]" << endl;
		cout << "TriggerCloseThresholds            : ["
				<< CxxUtilities::String::join(this->TriggerCloseThresholds, ", ") << "]" << endl;
		cout << endl;

		cout << "//---------------------------------------------" << endl;
		cout << "// Programing the digitizer" << endl;
		cout << "//---------------------------------------------" << endl;
		try {
			//record length
			this->setNumberOfSamples(PreTriggerSamples + PostTriggerSamples);
			this->setNumberOfSamplesInEventPacket(SamplesInEventPacket);
			for (size_t ch = 0; ch < nChannels; ch++) {

				//pre-trigger (delay)
				this->setDepthOfDelay(ch, PreTriggerSamples);

				//trigger mode
				const auto triggerMode = this->TriggerModes.at(ch);
				this->setTriggerMode(ch, triggerMode);

				//threshold
				this->setStartingThreshold(ch, TriggerThresholds[ch]);
				this->setClosingThreshold(ch, TriggerCloseThresholds[ch]);

				//adc clock 50MHz
				this->setAdcClock(SpaceFibreADC::ADCClockFrequency::ADCClock50MHz);

				//turn on ADC
				this->turnOnADCPower(ch);

			}
			cout << "Device configurtion done." << endl;
		} catch (...) {
			cerr << "Device configuration failed." << endl;
			::exit(-1);
		}
	}
};

#endif /* GROWTH_FY2015_ADC_HH_ */
