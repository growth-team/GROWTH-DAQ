#ifndef SPACEWIRE_SPACEWIREIFOVERTCP_HH_
#define SPACEWIRE_SPACEWIREIFOVERTCP_HH_

#include "CxxUtilities/CxxUtilities.hh"

#include "SpaceWireIF.hh"
#include "SpaceWireUtilities.hh"
#include "SpaceWireSSDTPModule.hh"

class SpaceWireIFOverTCP: public SpaceWireIF, public SpaceWireIFActionTimecodeScynchronizedAction {
private:
	bool opened;
public:
	enum {
		ClientMode, ServerMode
	};

private:
	std::string iphostname;
	u32 portnumber;
	SpaceWireSSDTPModule* ssdtp;
	CxxUtilities::TCPSocket* datasocket;
	CxxUtilities::TCPServerSocket* serverSocket;

	u32 operationMode;

public:
	/** Constructor (client mode).
	 */
	SpaceWireIFOverTCP(std::string iphostname, u32 portnumber) :
			SpaceWireIF(), iphostname(iphostname), portnumber(portnumber) {
		setOperationMode(ClientMode);
	}

	/** Constructor (server mode).
	 */
	SpaceWireIFOverTCP(u32 portnumber) :
			SpaceWireIF(), portnumber(portnumber) {
		setOperationMode(ServerMode);
	}

	/** Constructor. Server/client mode will be determined later via
	 * the setClientMode() or setServerMode() method.
	 */
	SpaceWireIFOverTCP() :
			SpaceWireIF() {

	}

	virtual ~SpaceWireIFOverTCP() {
	}

public:
	void setClientMode(std::string iphostname, u32 portnumber) {
		setOperationMode(ClientMode);
		this->iphostname = iphostname;
		this->portnumber = portnumber;
	}

	void setServerMode(u32 portnumber) {
		setOperationMode(ServerMode);
		this->portnumber = portnumber;
	}

public:
	void open() throw (SpaceWireIFException) {
		using namespace std;
		using namespace std;
		using namespace CxxUtilities;
		if (isClientMode()) { //client mode
			datasocket = NULL;
			ssdtp = NULL;
			try {
				datasocket = new TCPClientSocket(iphostname, portnumber);
				((TCPClientSocket*) datasocket)->open(1000);
				setTimeoutDuration(500000);
			} catch (CxxUtilities::TCPSocketException& e) {
				throw SpaceWireIFException(SpaceWireIFException::OpeningConnectionFailed);
			} catch (...) {
				throw SpaceWireIFException(SpaceWireIFException::OpeningConnectionFailed);
			}
		} else { //server mode
			using namespace std;
			datasocket = NULL;
			ssdtp = NULL;
			try {
				serverSocket = new TCPServerSocket(portnumber);
				serverSocket->open();
				datasocket = serverSocket->accept();
				setTimeoutDuration(500000);
			} catch (CxxUtilities::TCPSocketException& e) {
				throw SpaceWireIFException(SpaceWireIFException::OpeningConnectionFailed);
			} catch (...) {
				throw SpaceWireIFException(SpaceWireIFException::OpeningConnectionFailed);
			}
		}
		datasocket->setNoDelay();
		ssdtp = new SpaceWireSSDTPModule(datasocket);
		ssdtp->setTimeCodeAction(this);
		state = Opened;
	}

	void close() throw (SpaceWireIFException) {
		using namespace CxxUtilities;
		using namespace std;
		if (state == Closed) {
			return;
		}
		state = Closed;
		//invoke SpaceWireIFCloseActions to tell other instances
		//closing of this SpaceWire interface
		invokeSpaceWireIFCloseActions();
		if (ssdtp != NULL) {
			delete ssdtp;
		}
		ssdtp = NULL;
		if (datasocket != NULL) {
			if (isClientMode()) {
				((TCPClientSocket*) datasocket)->close();
				delete (TCPClientSocket*) datasocket;
			} else {
				((TCPServerAcceptedSocket*) datasocket)->close();
				delete (TCPServerAcceptedSocket*) datasocket;
			}
		}
		datasocket = NULL;
		if (isServerMode()) {
			if (serverSocket != NULL) {
				serverSocket->close();
				delete serverSocket;
			}
		}
	}

public:
	void send(u8* data, size_t length, SpaceWireEOPMarker::EOPType eopType = SpaceWireEOPMarker::EOP)
			throw (SpaceWireIFException) {
		using namespace std;
		if (ssdtp == NULL) {
			throw SpaceWireIFException(SpaceWireIFException::LinkIsNotOpened);
		}
		try {
			ssdtp->send(data, length, eopType);
		} catch (SpaceWireSSDTPException& e) {
			if (e.getStatus() == SpaceWireSSDTPException::Timeout) {
				throw SpaceWireIFException(SpaceWireIFException::Timeout);
			} else {
				throw SpaceWireIFException(SpaceWireIFException::Disconnected);
			}
		}
	}

public:
	void receive(std::vector<u8>* buffer) throw (SpaceWireIFException) {
		if (ssdtp == NULL) {
			throw SpaceWireIFException(SpaceWireIFException::LinkIsNotOpened);
		}
		try {
			u32 eopType;
			ssdtp->receive(buffer, eopType);
			if (eopType == SpaceWireEOPMarker::EEP) {
				this->setReceivedPacketEOPMarkerType(SpaceWireIF::EEP);
				if (this->eepShouldBeReportedAsAnException_) {
					throw SpaceWireIFException(SpaceWireIFException::EEP);
				}
			} else {
				this->setReceivedPacketEOPMarkerType(SpaceWireIF::EOP);
			}
		} catch (SpaceWireSSDTPException& e) {
			if (e.getStatus() == SpaceWireSSDTPException::Timeout) {
				throw SpaceWireIFException(SpaceWireIFException::Timeout);
			}
			using namespace std;
			throw SpaceWireIFException(SpaceWireIFException::Disconnected);
		} catch (CxxUtilities::TCPSocketException& e) {
			if (e.getStatus() == CxxUtilities::TCPSocketException::Timeout) {
				throw SpaceWireIFException(SpaceWireIFException::Timeout);
			}
			using namespace std;
			throw SpaceWireIFException(SpaceWireIFException::Disconnected);
		}
	}

public:
	void emitTimecode(u8 timeIn, u8 controlFlagIn = 0x00) throw (SpaceWireIFException) {
		using namespace std;
		if (ssdtp == NULL) {
			throw SpaceWireIFException(SpaceWireIFException::LinkIsNotOpened);
		}
		timeIn = timeIn % 64 + (controlFlagIn << 6);
		try {
			//emit timecode via SSDTP module
			ssdtp->sendTimeCode(timeIn);
		} catch (SpaceWireSSDTPException& e) {
			if (e.getStatus() == SpaceWireSSDTPException::Timeout) {
				throw SpaceWireIFException(SpaceWireIFException::Timeout);
			}
			throw SpaceWireIFException(SpaceWireIFException::Disconnected);
		} catch (CxxUtilities::TCPSocketException& e) {
			if (e.getStatus() == CxxUtilities::TCPSocketException::Timeout) {
				throw SpaceWireIFException(SpaceWireIFException::Timeout);
			}
			throw SpaceWireIFException(SpaceWireIFException::Disconnected);
		}
		//invoke timecode synchronized action
		if (timecodeSynchronizedActions.size() != 0) {
			this->invokeTimecodeSynchronizedActions(timeIn);
		}
	}

public:
	virtual void setTxLinkRate(u32 linkRateType) throw (SpaceWireIFException) {
		using namespace std;
		cerr << "SpaceWireIFOverIPClient::setTxLinkRate() is not implemented." << endl;
		cerr << "Please use SpaceWireIFOverIPClient::setTxDivCount() instead." << endl;
		throw SpaceWireIFException(SpaceWireIFException::FunctionNotImplemented);
	}

	virtual u32 getTxLinkRateType() throw (SpaceWireIFException) {
		using namespace std;
		cerr << "SpaceWireIFOverIPClient::getTxLinkRate() is not implemented." << endl;
		throw SpaceWireIFException(SpaceWireIFException::FunctionNotImplemented);
	}

public:
	void setTxDivCount(u8 txdivcount) {
		if (ssdtp == NULL) {
			throw SpaceWireIFException(SpaceWireIFException::LinkIsNotOpened);
		}
		try {
			ssdtp->setTxDivCount(txdivcount);
		} catch (SpaceWireSSDTPException& e) {
			if (e.getStatus() == SpaceWireSSDTPException::Timeout) {
				throw SpaceWireIFException(SpaceWireIFException::Timeout);
			}
			throw SpaceWireIFException(SpaceWireIFException::Disconnected);
		} catch (CxxUtilities::TCPSocketException& e) {
			if (e.getStatus() == CxxUtilities::TCPSocketException::Timeout) {
				throw SpaceWireIFException(SpaceWireIFException::Timeout);
			}
			throw SpaceWireIFException(SpaceWireIFException::Disconnected);
		}
	}

public:
	void setTimeoutDuration(double microsecond) throw (SpaceWireIFException) {
		datasocket->setTimeout(microsecond / 1000.);
		timeoutDurationInMicroSec = microsecond;
	}

public:
	u8 getTimeCode() throw (SpaceWireIFException) {
		if (ssdtp == NULL) {
			throw SpaceWireIFException(SpaceWireIFException::LinkIsNotOpened);
		}
		return ssdtp->getTimeCode();
	}

	void doAction(u8 timecode) {
		this->invokeTimecodeSynchronizedActions(timecode);
	}

public:
	SpaceWireSSDTPModule* getSSDTPModule() {
		return ssdtp;
	}

	u32 getOperationMode() const {
		return operationMode;
	}

	void setOperationMode(u32 operationMode) {
		this->operationMode = operationMode;
	}

	bool isClientMode() {
		if (operationMode == ClientMode) {
			return true;
		} else {
			return false;
		}
	}

	bool isServerMode() {
		if (operationMode != ClientMode) {
			return true;
		} else {
			return false;
		}
	}

public:
	/** Cancels ongoing receive() method if any exist.
	 */
	void cancelReceive(){}
};


#endif
