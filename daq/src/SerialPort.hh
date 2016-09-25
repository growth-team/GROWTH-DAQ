/*
 * SerialPort.hh
 *
 *  Created on: Jun 9, 2015
 *      Author: yuasa
 */

#ifndef SERIALPORT_HH_
#define SERIALPORT_HH_

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/optional.hpp>
#include <boost/system/system_error.hpp>
//#include <boost/bind1st.hpp>
#include <boost/thread.hpp>
#include <vector>
#include <functional>
#include <algorithm>
#include <iostream>

#include "SpaceWireRMAPLibrary/SpaceWireUtilities.hh"

#include "CxxUtilities/CxxUtilities.hh"

class SerialPortException: public CxxUtilities::Exception {
public:
	enum {
		Timeout, SerialPortClosed, TooLargeDataReceived
	};

public:
	SerialPortException(int status) :
			CxxUtilities::Exception(status) {
	}

public:
	std::string toString() {
		switch (this->getStatus()) {
		case Timeout:
			return "Timeout";
		case SerialPortClosed:
			return "SerialPortClosed";
		case TooLargeDataReceived:
			return "TooLargeDataReceived";
		default:
			return "UndefinedException";
		}
	}
};

struct SerialPort {
public:
	static const size_t ReadBufferSize = 10 * 1024;

public:
	void read_callback(const boost::system::error_code &e, std::size_t size, boost::array<char, ReadBufferSize> r) {
		std::cout << "read :" << size << "byte[s] " << std::endl;
		std::cout.write(r.data(), size);
	}

public:
	void write_callback(const boost::system::error_code &e, std::size_t size) {
		std::cout << "write :" << size << "byte[s]" << std::endl;
	}

private:
	boost::asio::serial_port *port;
	boost::asio::io_service io;
	boost::asio::streambuf receive_buffer_;

public:
	SerialPort(std::string deviceName, size_t baudRate = 9600) :
			timeoutDurationObject(1000) {
		using namespace std;

		port = new boost::asio::serial_port(io, deviceName.c_str());
		port->set_option(boost::asio::serial_port_base::baud_rate(baudRate));
		port->set_option(boost::asio::serial_port_base::character_size(8));
		port->set_option(boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none));
		port->set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none));
		port->set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));

		// for non-blocking
		// boost::thread thr_io(boost::bind(&boost::asio::io_service::run, &io));

		setTimeout(1000);
	}

	/*
	 public:
	 size_t receive(std::vector<uint8_t> &receiveBuffer) {
	 return port->read_some(boost::asio::buffer(receiveBuffer));
	 }*/

public:
	void close() {
		port->close();
		delete port;
	}

public:
	void send(std::vector<uint8_t> &sendBuffer) {
#ifdef DEBUG_SERIALPORT
		using namespace std;
		cout << "Send: ";
		SpaceWireUtilities::dumpPacket(sendBuffer);
#endif
		port->write_some(boost::asio::buffer(sendBuffer));
	}

private:
	std::vector<uint8_t> internalBufferForSend;

public:
	void send(uint8_t* sendBuffer, size_t length) {
		internalBufferForSend.resize(length);
		memcpy(&(internalBufferForSend[0]), sendBuffer, length);
#ifdef DEBUG_SERIALPORT
		using namespace std;
		cout << "Send: ";
		SpaceWireUtilities::dumpPacket(internalBufferForSend);
#endif
		port->write_some(boost::asio::buffer(internalBufferForSend));
	}

	/*
	 public:
	 void write_nonblocking(uint8_t *buffer, size_t length) {
	 port->async_write_some(
	 boost::asio::buffer(buffer, length),
	 boost::bind(&SerialPort::write_callback, this, _1, _2));
	 }

	 public:
	 std::vector<uint8_t> read_nonblocking() {
	 port->async_read_some(//
	 receive_buffer_, //
	 boost::asio::transfer_at_least(1),//
	 boost::bind(&SerialPort::read_callback, this)//
	 );
	 std::vector<uint8_t> result;
	 return result;
	 }*/

private:
	CxxUtilities::Mutex mutex;

public:
	template<typename MutableBufferSequence>
	void receiveWithTimeout(const MutableBufferSequence &buffers, size_t &nReceivedBytes,
			const boost::asio::deadline_timer::duration_type &expiry_time) {
		mutex.lock();
		boost::optional<boost::system::error_code> timer_result;
		boost::asio::deadline_timer timer(port->get_io_service());
		timer.expires_from_now(expiry_time);
		timer.async_wait([&timer_result](const boost::system::error_code &error) {
			timer_result.reset(error);
		});

		boost::optional<boost::system::error_code> read_result;
		port->async_read_some(buffers, [&read_result,&nReceivedBytes](const boost::system::error_code &error,
				size_t _nReceivedBytes) {read_result.reset(error); nReceivedBytes=_nReceivedBytes;});

		port->get_io_service().reset();
		while (port->get_io_service().run_one()) {
			if (read_result)
				timer.cancel();
			else if (timer_result)
				port->cancel();
		}

		if (*read_result) {
			mutex.unlock();
			throw boost::system::system_error(*read_result);
		}

		mutex.unlock();
	}

public:
	/** Receives data until specified length is completely received.
	 * Received data are stored in the data buffer.
	 * @param[in] data uint8_t buffer where received data will be stored
	 * @param[in] length length of data to be received
	 * @return received size
	 */
	inline size_t receiveLoopUntilSpecifiedLengthCompletes(uint8_t* data, uint32_t length) throw (SerialPortException) {
		return receive(data, length, true);
	}

public:
	/** Receives data. The maximum length can be specified as length.
	 * Receive size may be shorter than the specified length.
	 * @param[in] data uint8_t buffer where received data will be stored
	 * @param[in] length the maximum size of the data buffer
	 * @return received size
	 */
	size_t receive(uint8_t* data, uint32_t length, bool waitUntilSpecifiedLengthCompletes = false)
			throw (SerialPortException) {
		int result = 0;
		int remainingLength = length;
		int readDoneLength = 0;
		size_t nReceivedBytes;

		try {
			_SerialPort_receive_loop: //
			receiveWithTimeout(boost::asio::buffer((uint8_t*) data + readDoneLength, remainingLength), nReceivedBytes,
					timeoutDurationObject);

			if (nReceivedBytes > length) {
				using namespace std;
				cerr << "SerialPort::receive(): too long data (" << nReceivedBytes << " bytes) received against specified "
						<< length << " bytes" << endl;
				throw SerialPortException(SerialPortException::TooLargeDataReceived);
			}

			if (waitUntilSpecifiedLengthCompletes == false) {
				return nReceivedBytes;
			} else {
				remainingLength = remainingLength - nReceivedBytes;
				readDoneLength = readDoneLength + nReceivedBytes;
				if (remainingLength == 0) {
					return length;
				}
				goto _SerialPort_receive_loop;
			}
		} catch (boost::system::system_error& e) {
#ifdef DEBUG_SERIALPORT
			using namespace std;
			cout << "Exception in SerialPort::receive(): " << e.what() << endl;
#endif
			throw SerialPortException(SerialPortException::Timeout);
		}
	}

private:
	double timeoutDurationInMilliSec = 1000;
	boost::posix_time::millisec timeoutDurationObject;

public:
	void setTimeout(double timeoutDurationInMilliSec) {
		this->timeoutDurationInMilliSec = timeoutDurationInMilliSec;
		timeoutDurationObject = boost::posix_time::millisec(timeoutDurationInMilliSec);
	}
};

#endif /* SERIALPORT_HH_ */
