#ifndef SPACEWIRE_SPACEWIREIFOVERTCP_HH_
#define SPACEWIRE_SPACEWIREIFOVERTCP_HH_

#include "spacewire/spacewireif.hh"
#include "spacewire/spacewiressdtpmodule.hh"
#include "spacewire/spacewireutil.hh"
#include "spacewire/tcpsocket.hh"
#include "spacewire/types.hh"

class SpaceWireIFOverTCP : public SpaceWireIF {
 public:
  enum class ConnectionMode { ClientMode, ServerMode };

 public:
  SpaceWireIFOverTCP(const std::string& iphostname, u32 portnumber)
      : iphostname(iphostname), portnumber(portnumber), operationMode(ConnectionMode::ClientMode) {}
  SpaceWireIFOverTCP(u32 portnumber) : portnumber(portnumber), operationMode(ConnectionMode::ServerMode) {}
  virtual ~SpaceWireIFOverTCP() = default;

  void open() override {
    if (isClientMode()) {  // client mode
      datasocket = NULL;
      ssdtp = NULL;
      try {
        datasocket = new TCPClientSocket(iphostname, portnumber);
        ((TCPClientSocket*)datasocket)->open(1000);
        setTimeoutDuration(500000);
      } catch (TCPSocketException& e) {
        throw SpaceWireIFException(SpaceWireIFException::OpeningConnectionFailed);
      } catch (...) {
        throw SpaceWireIFException(SpaceWireIFException::OpeningConnectionFailed);
      }
    } else {  // server mode
      datasocket = NULL;
      ssdtp = NULL;
      try {
        serverSocket = new TCPServerSocket(portnumber);
        serverSocket->open();
        datasocket = serverSocket->accept();
        setTimeoutDuration(500000);
      } catch (TCPSocketException& e) {
        throw SpaceWireIFException(SpaceWireIFException::OpeningConnectionFailed);
      } catch (...) {
        throw SpaceWireIFException(SpaceWireIFException::OpeningConnectionFailed);
      }
    }
    datasocket->setNoDelay();
    ssdtp = new SpaceWireSSDTPModule(datasocket);
    state = Opened;
  }

  void close() override {
    if (state == Closed) {
      return;
    }
    state = Closed;
    // invoke SpaceWireIFCloseActions to tell other instances
    // closing of this SpaceWire interface
    invokeSpaceWireIFCloseActions();
    if (ssdtp != NULL) {
      delete ssdtp;
    }
    ssdtp = NULL;
    if (datasocket != NULL) {
      if (isClientMode()) {
        ((TCPClientSocket*)datasocket)->close();
        delete (TCPClientSocket*)datasocket;
      } else {
        ((TCPServerAcceptedSocket*)datasocket)->close();
        delete (TCPServerAcceptedSocket*)datasocket;
      }
    }
    datasocket = NULL;
    if (!isClientMode()) {
      if (serverSocket != NULL) {
        serverSocket->close();
        delete serverSocket;
      }
    }
  }

  void send(u8* data, size_t length, EOPType eopType = EOP) {
    if (ssdtp == NULL) {
      throw SpaceWireIFException(SpaceWireIFException::LinkIsNotOpened);
    }
    try {
      ssdtp->send(data, length, eopType);
    } catch (const SpaceWireSSDTPException& e) {
      if (e.getStatus() == SpaceWireSSDTPException::Timeout) {
        throw SpaceWireIFException(SpaceWireIFException::Timeout);
      } else {
        throw SpaceWireIFException(SpaceWireIFException::Disconnected);
      }
    }
  }

  void receive(std::vector<u8>* buffer) {
    if (ssdtp == NULL) {
      throw SpaceWireIFException(SpaceWireIFException::LinkIsNotOpened);
    }
    try {
      u32 eopType;
      ssdtp->receive(buffer, eopType);
      if (eopType == EEP) {
        this->setReceivedPacketEOPMarkerType(SpaceWireIF::EEP);
        if (this->eepShouldBeReportedAsAnException_) {
          throw SpaceWireIFException(SpaceWireIFException::EEP);
        }
      } else {
        this->setReceivedPacketEOPMarkerType(SpaceWireIF::EOP);
      }
    } catch (const SpaceWireSSDTPException& e) {
      if (e.getStatus() == SpaceWireSSDTPException::Timeout) {
        throw SpaceWireIFException(SpaceWireIFException::Timeout);
      }
      throw SpaceWireIFException(SpaceWireIFException::Disconnected);
    }
  }

  void setTimeoutDuration(f64 microsecond) override {
    datasocket->setTimeout(microsecond / 1000.);
    timeoutDurationInMicroSec = microsecond;
  }

  SpaceWireSSDTPModule* getSSDTPModule() { return ssdtp; }
  u32 getOperationMode() const { return operationMode; }
  bool isClientMode() const { return operationMode == ConnectionMode::ClientMode; }
  void cancelReceive() override {}

 private:
  std::string iphostname{};
  u32 portnumber{};
  SpaceWireSSDTPModule* ssdtp{};
  TCPSocket* datasocket{};
  TCPServerSocket* serverSocket{};

  u32 operationMode;
};

#endif
