/*
 ============================================================================
 SpaceWire/RMAP Library is provided under the MIT License.
 ============================================================================

 Copyright (c) 2006-2013 Takayuki Yuasa and The Open-source SpaceWire Project

 Permission is hereby granted, free of charge, to any person obtaining a
 copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:

 The above copyright notice and this permission notice shall be included
 in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef SPACEWIRESSDTPMODULEUART_HH_
#define SPACEWIRESSDTPMODULEUART_HH_

#include "CxxUtilities/CommonHeader.hh"
#include "CxxUtilities/Exception.hh"
#include "CxxUtilities/Mutex.hh"
#include "CxxUtilities/Condition.hh"

#include "SpaceWireRMAPLibrary/SpaceWireIF.hh"
#include "SpaceWireRMAPLibrary/SpaceWireSSDTPModule.hh"

#include "SerialPort.hh"

/** A class that performs synchronous data transfer via
 * UART using "Simple- Synchronous- Data Transfer Protocol".
 */
class SpaceWireSSDTPModuleUART {
public:
	static const uint32_t BufferSize = 10 * 1024;

private:
	bool closed = false;

private:
	SerialPort* serialPort;
	uint8_t* sendbuffer;
	uint8_t* receivebuffer;
	std::stringstream ss;
	uint8_t internal_timecode;
	uint32_t latest_sentsize;
	CxxUtilities::Mutex sendmutex;
	CxxUtilities::Mutex receivemutex;
	SpaceWireIFActionTimecodeScynchronizedAction* timecodeaction;

private:
	/* for SSDTP2 */
	CxxUtilities::Mutex registermutex;
	CxxUtilities::Condition registercondition;
	std::map<uint32_t, uint32_t> registers;

private:
	uint8_t rheader[12];
	uint8_t sheader[12];

public:
	size_t receivedsize;
	size_t rbuf_index;

public:
	/** Constructor. */
	SpaceWireSSDTPModuleUART(SerialPort* serialPort) {
		this->serialPort = serialPort;
		sendbuffer = (uint8_t*) malloc(SpaceWireSSDTPModule::BufferSize);
		receivebuffer = (uint8_t*) malloc(SpaceWireSSDTPModule::BufferSize);
		internal_timecode = 0x00;
		latest_sentsize = 0;
		timecodeaction = NULL;
		rbuf_index = 0;
		receivedsize = 0;
		closed = false;
	}

public:
	/** Destructor. */
	~SpaceWireSSDTPModuleUART() {
		if (sendbuffer != NULL) {
			free(sendbuffer);
		}
		if (receivebuffer != NULL) {
			free(receivebuffer);
		}
	}

public:
	/** Sends a SpaceWire packet via the SpaceWire interface.
	 * This is a blocking method.
	 * @param[in] data packet content.
	 * @param[in] eopType End-of-Packet marker. SpaceWireEOPMarker::EOP or SpaceWireEOPMarker::EEP.
	 */
	void send(std::vector<uint8_t>& data, uint32_t eopType = SpaceWireEOPMarker::EOP) throw (SpaceWireSSDTPException) {
		sendmutex.lock();
		send(&data, eopType);
		sendmutex.unlock();
	}

public:
	/** Sends a SpaceWire packet via the SpaceWire interface.
	 * This is a blocking method.
	 * @param[in] data packet content.
	 * @param[in] eopType End-of-Packet marker. SpaceWireEOPMarker::EOP or SpaceWireEOPMarker::EEP.
	 */
	void send(std::vector<uint8_t>* data, uint32_t eopType = SpaceWireEOPMarker::EOP) throw (SpaceWireSSDTPException) {
		sendmutex.lock();
		if (this->closed) {
			sendmutex.unlock();
			return;
		}
		size_t size = data->size();
		if (eopType == SpaceWireEOPMarker::EOP) {
			sheader[0] = DataFlag_Complete_EOP;
		} else if (eopType == SpaceWireEOPMarker::EEP) {
			sheader[0] = DataFlag_Complete_EEP;
		} else if (eopType == SpaceWireEOPMarker::Continued) {
			sheader[0] = DataFlag_Flagmented;
		}
		sheader[1] = 0x00;
		for (uint32_t i = 11; i > 1; i--) {
			sheader[i] = size % 0x100;
			size = size / 0x100;
		}
		try {
			serialPort->send(sheader, 12);
			serialPort->send(&(data->at(0)), data->size());
#ifdef DEBUG_SSDTP
			using namespace std;
			size_t length = data->size();
			cout << "SSDTP::send():" << endl;
			cout << "Header: ";
			for (size_t i = 0; i < 12; i++) {
				cout << hex << right << setw(2) << setfill('0') << (uint32_t) sheader[i] << " ";
			}
			cout << endl;
			cout << "Data: " << dec << length << " bytes" << endl;
			for (size_t i = 0; i < length; i++) {
				cout << hex << right << setw(2) << setfill('0') << (uint32_t) data->at(i) << " ";
			}
			cout << endl << dec;
#endif
		} catch (...) {
			sendmutex.unlock();
			throw SpaceWireSSDTPException(SpaceWireSSDTPException::Disconnected);
		}
		sendmutex.unlock();
	}

public:
	/** Sends a SpaceWire packet via the SpaceWire interface.
	 * This is a blocking method.
	 * @param[in] data packet content.
	 * @param[in] the length length of the packet.
	 * @param[in] eopType End-of-Packet marker. SpaceWireEOPMarker::EOP or SpaceWireEOPMarker::EEP.
	 */
	void send(uint8_t* data, size_t length, uint32_t eopType = SpaceWireEOPMarker::EOP) throw (SpaceWireSSDTPException) {
		sendmutex.lock();
		if (this->closed) {
			sendmutex.unlock();
			return;
		}
		if (eopType == SpaceWireEOPMarker::EOP) {
			sheader[0] = DataFlag_Complete_EOP;
		} else if (eopType == SpaceWireEOPMarker::EEP) {
			sheader[0] = DataFlag_Complete_EEP;
		} else if (eopType == SpaceWireEOPMarker::Continued) {
			sheader[0] = DataFlag_Flagmented;
		}
		sheader[1] = 0x00;
		size_t asize = length;
		for (size_t i = 11; i > 1; i--) {
			sheader[i] = asize % 0x100;
			asize = asize / 0x100;
		}
		try {
			serialPort->send(sheader, 12);
			serialPort->send(data, length);
#ifdef DEBUG_SSDTP
			using namespace std;
			cout << "SSDTP::send():" << endl;
			cout << "Header: ";
			for (size_t i = 0; i < 12; i++) {
				cout << hex << right << setw(2) << setfill('0') << (uint32_t) sheader[i] << " ";
			}
			cout << endl;
			cout << "Data: " << dec << length << " bytes" << endl;
			for (size_t i = 0; i < length; i++) {
				cout << hex << right << setw(2) << setfill('0') << (uint32_t) data[i] << " ";
			}
			cout << endl << dec;
#endif
		} catch (...) {
			sendmutex.unlock();
			throw SpaceWireSSDTPException(SpaceWireSSDTPException::Disconnected);
		}
		sendmutex.unlock();
	}

public:
	/** Tries to receive a pcket from the SpaceWire interface.
	 * This method will block the thread for a certain length of time.
	 * Timeout can happen via the TCPSocket provided to the instance.
	 * The code below shows how the timeout duration can be changed.
	 * @code
	 * TCPClientSocket* socket=new TCPClientSocket("192.168.1.100", 10030);
	 * socket->open();
	 * socket->setTimeoutDuration(1000); //ms
	 * SpaceWireSSDTPModule* ssdtpModule=new SpaceWireSSDTPModule(socket);
	 * try{
	 * 	ssdtpModule->receive();
	 * }catch(SpaceWireSSDTPException& e){
	 * 	if(e.getStatus()==SpaceWireSSDTPException::Timeout){
	 * 	 std::cout << "Timeout" << std::endl;
	 * 	}else{
	 * 	 throw e;
	 * 	}
	 * }
	 * @endcode
	 * @returns packet content.
	 */
	std::vector<uint8_t> receive() throw (SpaceWireSSDTPException) {
		receivemutex.lock();
		if (this->closed) {
			receivemutex.unlock();
			return {};
		}
		std::vector<uint8_t> data;
		uint32_t eopType;
		receive(&data, eopType);
		receivemutex.unlock();
		return data;
	}

private:
	uint8_t rheader_previous[12];
	std::vector<uint8_t> receivedDataPrevious;

public:
	/** Tries to receive a pcket from the SpaceWire interface.
	 * This method will block the thread for a certain length of time.
	 * Timeout can happen via the TCPSocket provided to the instance.
	 * The code below shows how the timeout duration can be changed.
	 * @code
	 * TCPClientSocket* socket=new TCPClientSocket("192.168.1.100", 10030);
	 * socket->open();
	 * socket->setTimeoutDuration(1000); //ms
	 * SpaceWireSSDTPModule* ssdtpModule=new SpaceWireSSDTPModule(socket);
	 * try{
	 * 	ssdtpModule->receive();
	 * }catch(SpaceWireSSDTPException& e){
	 * 	if(e.getStatus()==SpaceWireSSDTPException::Timeout){
	 * 	 std::cout << "Timeout" << std::endl;
	 * 	}else{
	 * 	 throw e;
	 * 	}
	 * }
	 * @endcode
	 * @param[out] data a vector instance which is used to store received data.
	 * @param[out] eopType contains an EOP marker type (SpaceWireEOPMarker::EOP or SpaceWireEOPMarker::EEP).
	 */
	int receive(std::vector<uint8_t>* data, uint32_t& eopType) throw (SpaceWireSSDTPException) {
		size_t size = 0;
		size_t hsize = 0;
		size_t flagment_size = 0;
		size_t received_size = 0;

		try {
			using namespace std;
//		cout << "#1" << endl;
			receivemutex.lock();
			//header
			receive_header: //
			rheader[0] = 0xFF;
			rheader[1] = 0x00;
			while (rheader[0] != DataFlag_Complete_EOP && rheader[0] != DataFlag_Complete_EEP) {
//			cout << "#2" << endl;

//			cout << "#2-1" << endl;
				hsize = 0;
				flagment_size = 0;
				received_size = 0;
				//flag and size part
				try {
//				cout << "#2-2" << endl;
					while (hsize != 12) {
						if (this->closed) {
							return 0;
						}
						if (this->receiveCanceled) {
							//reset receiveCanceled
							this->receiveCanceled = false;
							//return with no data
							return 0;
						}
//					cout << "#2-3" << endl;
#ifdef DEBUG_SSDTP
						using namespace std;
						cout << "SSDTP::receive(): receiving header part (remaining size=" << dec << 12 - hsize << " bytes)" << endl;
#endif
						long result = serialPort->receive(rheader + hsize, 12 - hsize);
						hsize += result;
					}
				} catch (SerialPortException& e) {
//				cout << "#2-5" << endl;
					if (e.getStatus() == SerialPortException::Timeout) {
#ifdef DEBUG_SSDTP
						using namespace std;
						cout << "SSDTP::receive(): serial port timed out." << endl;
#endif
//					cout << "#2-6" << endl;
						throw SpaceWireSSDTPException(SpaceWireSSDTPException::Timeout);
					} else {
//					cout << "#2-7" << endl;
						throw SpaceWireSSDTPException(SpaceWireSSDTPException::Disconnected);
					}
				} catch (...) {
					throw SpaceWireSSDTPException(SpaceWireSSDTPException::Disconnected);
				}

#ifdef DEBUG_SSDTP
				using namespace std;
				cout << "SSDTP::receive(): header part received" << endl;
				cout << "Header: ";
				for (size_t i = 0; i < 12; i++) {
					cout << hex << right << setw(2) << setfill('0') << (uint32_t) rheader[i] << " ";
				}
				cout << endl;
#endif

//			cout << "#3" << endl;
				//data or control code part
				if (rheader[0] == DataFlag_Complete_EOP || rheader[0] == DataFlag_Complete_EEP
						|| rheader[0] == DataFlag_Flagmented) {
//				cout << "#4" << endl;
					//data
					for (uint32_t i = 2; i < 12; i++) {
						flagment_size = flagment_size * 0x100 + rheader[i];
					}
					//verify flagment size
					if (flagment_size > BufferSize) {
						cerr << "SpaceWireSSDTPModuleUART::receive(): too large flagment size (" << flagment_size << " bytes)"
								<< endl;
						cerr << "Something is wrong with SSDTP over Serail Port data communication." << endl;
						cerr << "Trying to receive remainig data from receive buffer." << endl;
						long result = serialPort->receive(receivebuffer, BufferSize);
						cerr << result << " bytes received." << endl;
						cout << "Data: ";
						for (size_t i = 0; i < result; i++) {
							cout << hex << right << setw(2) << setfill('0') << (uint32_t) *(receivebuffer + i) << " ";
						}
						cout << endl;
						exit(-1);
					}

					uint8_t* data_pointer = receivebuffer;
//				cout << "#5" << endl;
					while (received_size != flagment_size) {
//					cout << "#6" << endl;
						long result;
						_loop_receiveDataPart: //
						if (this->receiveCanceled) {
							//reset receiveCanceled
							this->receiveCanceled = false;
							//dump info
							cerr << "SpaceWireSSDTPModuleUART::receive(): canceled while waiting to receive " << dec << flagment_size
									<< " bytes." << endl;
							cout << "Current header:" << endl;
							for (size_t i = 0; i < 12; i++) {
								cout << hex << right << setw(2) << setfill('0') << (uint32_t) rheader[i] << " ";
							}
							cout << dec << endl;
							//return with no data
							return 0;
						}
						try {
							result = serialPort->receive(data_pointer + size + received_size, flagment_size - received_size);
#ifdef DEBUG_SSDTP
							using namespace std;
							cout << "SSDTP::receive(): data part received" << endl;
							cout << "Data: ";
							for (size_t i = 0; i < result; i++) {
								cout << hex << right << setw(2) << setfill('0') << (uint32_t) *(data_pointer + size + received_size + i)
										<< " ";
							}
							cout << endl;
#endif
						} catch (SerialPortException e) {
							if (e.getStatus() == SerialPortException::Timeout) {
								goto _loop_receiveDataPart;
							}
							cout << "SSDTPModule::receive() exception when receiving data" << endl;
							cout << "rheader[0]=0x" << setw(2) << setfill('0') << hex << (uint32_t) rheader[0] << endl;
							cout << "rheader[1]=0x" << setw(2) << setfill('0') << hex << (uint32_t) rheader[1] << endl;
							cout << "size=" << dec << size << endl;
							cout << "flagment_size=" << dec << flagment_size << endl;
							cout << "received_size=" << dec << received_size << endl;
							exit(-1);
						}
						received_size += result;
					}
//				cout << "#7" << endl;
					size += received_size;
				} else if (rheader[0] == ControlFlag_SendTimeCode || rheader[0] == ControlFlag_GotTimeCode) {
					//control
					uint8_t timecode_and_reserved[2];
					uint32_t tmp_size = 0;
					try {
						while (tmp_size != 2) {
							int result = serialPort->receive(timecode_and_reserved + tmp_size, 2 - tmp_size);
							tmp_size += result;
						}
					} catch (...) {
						throw SpaceWireSSDTPException(SpaceWireSSDTPException::TCPSocketError);
					}
					switch (rheader[0]) {
					case ControlFlag_SendTimeCode:
						internal_timecode = timecode_and_reserved[0];
						gotTimeCode(internal_timecode);
						break;
					case ControlFlag_GotTimeCode:
						internal_timecode = timecode_and_reserved[0];
						gotTimeCode(internal_timecode);
						break;
					}
//				cout << "#11" << endl;
				} else {
					cout << "SSDTP fatal error with flag value of 0x" << hex << (uint32_t) rheader[0] << dec << endl;
					cout << "Previous data: (" << dec << receivedDataPrevious.size() << " bytes)" << endl;
					for (size_t i = 0; i < receivedDataPrevious.size(); i++) {
						cout << hex << right << setw(2) << setfill('0') << (uint32_t) receivedDataPrevious[i] << " ";
					}
					cout << dec << endl;
					cout << "Current header:" << endl;
					for (size_t i = 0; i < 12; i++) {
						cout << hex << right << setw(2) << setfill('0') << (uint32_t) rheader[i] << " ";
					}
					cout << dec << endl;
					throw SpaceWireSSDTPException(SpaceWireSSDTPException::TCPSocketError);
				}
			}
//		cout << "#8 " << size << endl;
			data->resize(size);
			if (size != 0) {
				memcpy(&(data->at(0)), receivebuffer, size);
			} else {
				goto receive_header;
			}
			if (rheader[0] == DataFlag_Complete_EOP) {
				eopType = SpaceWireEOPMarker::EOP;
			} else if (rheader[0] == DataFlag_Complete_EEP) {
				eopType = SpaceWireEOPMarker::EEP;
			} else {
				eopType = SpaceWireEOPMarker::Continued;
			}
//		cout << "#9" << endl;

			//debug receive data
#ifdef DEBUG_SSDTP
			static const size_t ReceivedDataPreviousMax = 4096;
			receivedDataPrevious.resize(std::min(size, ReceivedDataPreviousMax));
			memcpy(&(receivedDataPrevious[0]), receivebuffer, std::min(size, ReceivedDataPreviousMax));
#endif
			receivemutex.unlock();
			return size;
		} catch (SpaceWireSSDTPException& e) {
			receivemutex.unlock();
			throw e;
		} catch (SerialPortException& e) {
			using namespace std;
			receivemutex.unlock();
			throw SpaceWireSSDTPException(SpaceWireSSDTPException::TCPSocketError);
		}
	}

public:
	/** Emits a TimeCode.
	 * @param[in] timecode timecode value.
	 */
	void sendTimeCode(uint8_t timecode) throw (SpaceWireSSDTPException) {
		sendmutex.lock();
		sendbuffer[0] = SpaceWireSSDTPModule::ControlFlag_SendTimeCode;
		sendbuffer[1] = 0x00; //Reserved
		for (uint32_t i = 0; i < LengthOfSizePart - 1; i++) {
			sendbuffer[i + 2] = 0x00;
		}
		sendbuffer[11] = 0x02; //2bytes = 1byte timecode + 1byte reserved
		sendbuffer[12] = timecode;
		sendbuffer[13] = 0;
		try {
			serialPort->send(sendbuffer, 14);
			sendmutex.unlock();
		} catch (SerialPortException& e) {
			sendmutex.unlock();
			if (e.getStatus() == SerialPortException::Timeout) {
				throw SpaceWireSSDTPException(SpaceWireSSDTPException::Timeout);
			} else {
				throw SpaceWireSSDTPException(SpaceWireSSDTPException::Disconnected);
			}
		}
	}

public:
	/** Returns a time-code counter value.
	 * The time-code counter will be updated when a TimeCode is
	 * received from the SpaceWire interface. Receive of TimeCodes
	 * is not automatic, but performed in received() method silently.
	 * Therefore, to continuously receive TimeCodes, it is necessary to
	 * invoke receive() method repeatedly in, for example, a dedicated
	 * thread.
	 * @returns a locally stored TimeCode value.
	 */
	uint8_t getTimeCode() throw (SpaceWireSSDTPException) {
		return internal_timecode;
	}

public:
	/** Registers an action instance which will be invoked when a TimeCode is received.
	 * @param[in] SpaceWireIFActionTimecodeScynchronizedAction an action instance.
	 */
	void setTimeCodeAction(SpaceWireIFActionTimecodeScynchronizedAction* action) {
		timecodeaction = action;
	}

public:
	/** Sets link frequency.
	 * @attention This method is deprecated.
	 * @attention 4-port SpaceWire-to-GigabitEther does not provides this function since
	 * Link Speeds of external SpaceWire ports can be changed by updating
	 * dedicated registers available at Router Configuration port via RMAP.
	 */
	void setTxFrequency(double frequencyInMHz) {
		throw SpaceWireSSDTPException(SpaceWireSSDTPException::NotImplemented);
	}

public:
	/** An action method which is internally invoked when a TimeCode is received.
	 * This method is automatically called by SpaceWireSSDTP::receive() methods,
	 * and users do not need to use this method.
	 * @param[in] timecode a received TimeCode value.
	 */
	void gotTimeCode(uint8_t timecode) {
		using namespace std;
		if (timecodeaction != NULL) {
			timecodeaction->doAction(timecode);
		} else {
			/*	cout << "SSDTPModule::gotTimeCode(): Got TimeCode : " << hex << setw(2) << setfill('0')
			 << (uint32_t) timecode << dec << endl;*/
		}
	}

private:
	void registerRead(uint32_t address) {
		throw SpaceWireSSDTPException(SpaceWireSSDTPException::NotImplemented);
	}

private:
	void registerWrite(uint32_t address, std::vector<uint8_t> data) {
//send command
		sendmutex.lock();
		sendbuffer[0] = ControlFlag_RegisterAccess_WriteCommand;
		sendmutex.unlock();
	}

public:
	/** @attention This method can be used only with 1-port SpaceWire-to-GigabitEther
	 * (i.e. open-source version of SpaceWire-to-GigabitEther running with the ZestET1 FPGA board).
	 * @param[in] txdivcount link frequency will be 200/(txdivcount+1) MHz.
	 */
	void setTxDivCount(uint8_t txdivcount) {
		sendmutex.lock();
		sendbuffer[0] = SpaceWireSSDTPModule::ControlFlag_ChangeTxSpeed;
		sendbuffer[1] = 0x00; //Reserved
		for (uint32_t i = 0; i < LengthOfSizePart - 1; i++) {
			sendbuffer[i + 2] = 0x00;
		}
		sendbuffer[11] = 0x02; //2bytes = 1byte txdivcount + 1byte reserved
		sendbuffer[12] = txdivcount;
		sendbuffer[13] = 0;
		try {
			serialPort->send(sendbuffer, 14);
		} catch (SerialPortException& e) {
			sendmutex.unlock();
			if (e.getStatus() == SerialPortException::Timeout) {
				throw SpaceWireSSDTPException(SpaceWireSSDTPException::Timeout);
			} else {
				throw SpaceWireSSDTPException(SpaceWireSSDTPException::Disconnected);
			}
		}
		sendmutex.unlock();
	}

public:
	/** Sends raw byte array via the socket.
	 * @attention Users should not use this method.
	 */
	void sendRawData(uint8_t* data, size_t length) throw (SpaceWireSSDTPException) {
		sendmutex.lock();
		try {
			serialPort->send(data, length);
		} catch (SerialPortException& e) {
			sendmutex.unlock();
			if (e.getStatus() == SerialPortException::Timeout) {
				throw SpaceWireSSDTPException(SpaceWireSSDTPException::Timeout);
			} else {
				throw SpaceWireSSDTPException(SpaceWireSSDTPException::Disconnected);
			}
		}
		sendmutex.unlock();
	}

public:
	/** Finalizes the instance.
	 * If another thread is waiting in receive(), it will return with 0 (meaning 0 byte received),
	 * enabling the thread to stop.
	 */
	void close() {
		this->closed = true;
	}

public:
	/** Cancels ongoing receive() method if any exist.
	 */
	void cancelReceive() {
		using namespace std;
		cerr << "SpaceWireSSDTPModuleUART::cancelReceive() invoked" << endl;
		this->receiveCanceled = true;
	}

private:
	bool receiveCanceled = false;

public:
	/* for SSDTP2 */
	static const uint8_t DataFlag_Complete_EOP = 0x00;
	static const uint8_t DataFlag_Complete_EEP = 0x01;
	static const uint8_t DataFlag_Flagmented = 0x02;
	static const uint8_t ControlFlag_SendTimeCode = 0x30;
	static const uint8_t ControlFlag_GotTimeCode = 0x31;
	static const uint8_t ControlFlag_ChangeTxSpeed = 0x38;
	static const uint8_t ControlFlag_RegisterAccess_ReadCommand = 0x40;
	static const uint8_t ControlFlag_RegisterAccess_ReadReply = 0x41;
	static const uint8_t ControlFlag_RegisterAccess_WriteCommand = 0x50;
	static const uint8_t ControlFlag_RegisterAccess_WriteReply = 0x51;
	static const uint32_t LengthOfSizePart = 10;
};

#endif /* SPACEWIRESSDTPMODULEUART_HH_ */

/** History
 * 2008-06-xx file created (Takayuki Yuasa)
 * 2008-12-17 TimeCode implemented (Takayuki Yuasa)
 * 2015-06-09 UART support (Takayuki Yuasa)
 */
