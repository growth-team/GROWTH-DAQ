/*
 * SemaphoreRegister.hh
 *
 *  Created on: Dec 14, 2013
 *      Author: yuasa
 */

#ifndef SEMAPHOREREGISTER_HH_
#define SEMAPHOREREGISTER_HH_

#include "SpaceWireRMAPLibrary/Boards/SpaceFibreADCBoardModules/Types.hh"
#include "SpaceWireRMAPLibrary/Boards/SpaceFibreADCBoardModules/RMAPHandler.hh"

/** An internal class which represents a semaphore in
 * the VHDL logic.
 */
class SemaphoreRegister {
private:
	RMAPHandler* rmaphandler;
	RMAPTargetNode* adcboxRMAPNode;
	uint32_t address;

public:
	/** Constructor.
	 * @param rmaphandler instance of RMAPHandler which is used for this class to communicate SpaceWire ADC Box
	 * @param address an address of which semaphore this instance should handle
	 */
	SemaphoreRegister(RMAPHandler* handler, RMAPTargetNode* adcboxRMAPNode, uint32_t address) {
		using namespace std;
		this->rmaphandler = handler;
		this->address = address;
		this->adcboxRMAPNode = adcboxRMAPNode;
		if (Debug::semaphore()) {
			std::vector<uint8_t> data;
			data.push_back(0);
			data.push_back(0);
			rmaphandler->read(adcboxRMAPNode, address, 2, &data[0]);
			cout << "Semaphore::Semaphore(): semaphore (0x" << setw(8) << setfill('0') << hex << address
					<< ") was created, and value is " << data[1] * 0x100 + data[0] << endl << dec;
		}
		/* 20131214 comment out
		 request();
		 release();
		 */
	}

private:
	CxxUtilities::Condition condition;

private:
	uint16_t semaphoreValue;

public:
	/** Requests the semaphore. This method will wait for
	 * infinitely until it successfully gets the semaphore.
	 */
	void request() {
		using namespace std;
		if (Debug::semaphore()) {
			cout << "Semaphore::request(): request semaphore(0x" << hex << setw(8) << setfill('0') << address << ")..."
					<< endl;
		}

		bool flag = true;
		do {
			if (Debug::semaphore()) {
				cout << "Semaphore::request(): trying to get semaphore" << endl;
			}
			rmaphandler->setRegister(address, 0xFFFF);
			semaphoreValue = rmaphandler->getRegister(address);
			if (Debug::semaphore()) {
				cout << "Semaphore::request(): semaphore value = " << hex << setw(4) << setfill('0')
						<< (uint32_t) semaphoreValue << dec << endl;
			}
			if (semaphoreValue != 0x00) {
				//exit from this loop
				flag = false;
			} else {
				condition.wait(100); //wait 100ms
			}
		} while (flag);

		if (Debug::semaphore()) {
			cout << "Semaphore::request(): got semaphore" << endl;
		}
	}

	/** Release the semaphore.
	 */
	void release() {
		using namespace std;
		bool flag = true;
		do {
			if (Debug::semaphore()) {
				cout << "Semaphore::release(): release semaphore(" << hex << setw(4) << setfill('0') << address << ")..."
						<< endl;
			}

			rmaphandler->setRegister(address, 0x0000);
			semaphoreValue = rmaphandler->getRegister(address);
			if (Debug::semaphore()) {
				cout << "Semaphore::release(): semaphore value = " << hex << setw(4) << setfill('0')
						<< (uint32_t) semaphoreValue << dec << endl;
			}
			if (semaphoreValue == 0x0000) {
				//exit from this loop
				flag = false;
			} else {
				condition.wait(100); //wait 100ms
			}
		} while (flag);

		if (Debug::semaphore()) {
			cout << "Semaphore::release(): released" << endl;
		}
	}
};

#endif /* SEMAPHOREREGISTER_HH_ */
