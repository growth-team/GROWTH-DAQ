#ifndef SPACEWIRE_TCPSOCKET_HH_
#define SPACEWIRE_TCPSOCKET_HH_

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

#include <string>
#include <cmath>

#include "spacewire/types.hh"

class TCPSocketException : public Exception {
 public:
  enum {
    Disconnected,
    Timeout,
    TCPSocketError,
    PortNumberError,
    BindError,
    ListenError,
    AcceptException,
    OpenException,
    CreateException,
    HostEntryError,
    ConnectFailed,
    ConnectExceptionWhenChangingSocketModeToNonBlocking,
    ConnectExceptionWhenWaitingForConnection,
    ConnectExceptionNonBlockingConnectionImmediateluSucceeded,
    ConnectExceptionNonBlockingConnectionReturnedUnexpecedResult,
    ConnectExceptionWhenChangingSocketModeToBlocking,
    TimeoutDurationCannotBeSetToDisconnectedSocket,
    Undefined
  };

  TCPSocketException(u32 status) : Exception(status) {}

  std::string toString() const override {
    std::string result;
    switch (status_) {
      case Disconnected:
        result = "Disconnected";
        break;
      case Timeout:
        result = "Timeout";
        break;
      case TCPSocketError:
        result = "TCPSocketError";
        break;
      case PortNumberError:
        result = "PortNumberError";
        break;
      case BindError:
        result = "BindError";
        break;
      case ListenError:
        result = "ListenError";
        break;
      case AcceptException:
        result = "AcceptException";
        break;
      case OpenException:
        result = "OpenException";
        break;
      case CreateException:
        result = "CreateException";
        break;
      case HostEntryError:
        result = "HostEntryError";
        break;
      case ConnectFailed:
        result = "ConnectFailed";
        break;
      case ConnectExceptionWhenChangingSocketModeToNonBlocking:
        result = "ConnectExceptionWhenChangingSocketModeToNonBlocking";
        break;
      case ConnectExceptionWhenWaitingForConnection:
        result = "ConnectExceptionWhenWaitingForConnection";
        break;
      case ConnectExceptionNonBlockingConnectionImmediateluSucceeded:
        result = "ConnectExceptionNonBlockingConnectionImmediateluSucceeded";
        break;
      case ConnectExceptionNonBlockingConnectionReturnedUnexpecedResult:
        result = "ConnectExceptionNonBlockingConnectionReturnedUnexpecedResult";
        break;
      case ConnectExceptionWhenChangingSocketModeToBlocking:
        result = "ConnectExceptionWhenChangingSocketModeToBlocking";
        break;
      case TimeoutDurationCannotBeSetToDisconnectedSocket:
        result = "TimeoutDurationCannotBeSetToDisconnectedSocket";
        break;
      case Undefined:
        result = "Undefined";
        break;
      default:
        result = "Undefined status";
        break;
    }
    return result;
  }
};

class TCPSocket {
 public:
  enum class State { TCPSocketInitialized, TCPSocketCreated, TCPSocketBound, TCPSocketListening, TCPSocketConnected };

  TCPSocket() = default;
  TCPSocket(i16 port) : port(port) {}
  virtual ~TCPSocket() = default;

  /** Sends data stored in the data buffer for a specified length.
   * @param[in] data uint8_t buffer that contains sent data
   * @param[in] length data length in bytes
   * @return sent size
   */
  size_t send(const u8* data, size_t length) {
    const i32 result = ::send(socketdescriptor, reinterpret_cast<const void*>(data), length, 0);
    if (result < 0) {
      throw TCPSocketException(TCPSocketException::TCPSocketError);
    }
    return result;
  }

  /** Receives data until specified length is completely received.
   * Received data are stored in the data buffer.
   * @param[in] data uint8_t buffer where received data will be stored
   * @param[in] length length of data to be received
   * @return received size
   */
  size_t receiveLoopUntilSpecifiedLengthCompletes(u8* data, u32 length) { return receive(data, length, true); }

  /** Receives data. The maximum length can be specified as length.
   * Receive size may be shorter than the specified length.
   * @param[in] data uint8_t buffer where received data will be stored
   * @param[in] length the maximum size of the data buffer
   * @return received size
   */
  size_t receive(u8* data, u32 length, bool waitUntilSpecifiedLengthCompletes = false) {
    i32 result = 0;
    i32 remainingLength = length;
    i32 readDoneLength = 0;

  _tcpocket_receive_loop:  //
    result = ::recv(socketdescriptor, (reinterpret_cast<u8*>(data) + readDoneLength), remainingLength, 0);

    if (result <= 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        throw TCPSocketException(TCPSocketException::Timeout);
      } else {
        std::string err;
        switch (errno) {
          case EBADF:
            err = "EBADF";
            break;
          case ECONNREFUSED:
            err = "ECONNREFUSED";
            break;
          case EFAULT:
            err = "EFAULT";
            break;
          case EINTR:
            err = "EINTR";
            break;
          case EINVAL:
            err = "EINVAL";
            break;
          case ENOMEM:
            err = "ENOMEM";
            break;
          case ENOTCONN:
            err = "ENOTCONN";
            break;
          case ENOTSOCK:
            err = "ENOTSOCK";
            break;
        }
        // temporary fixing 20120809 Takayuki Yuasa for CentOS VM environment
        if (errno == EINTR) {
          goto _tcpocket_receive_loop;
        } else {
          usleep(1000000);
          throw TCPSocketException(TCPSocketException::Disconnected);
        }
      }
    }

    if (!waitUntilSpecifiedLengthCompletes) {
      return result;
    } else {
      remainingLength = remainingLength - result;
      readDoneLength = readDoneLength + result;
      if (remainingLength == 0) {
        return length;
      }
      goto _tcpocket_receive_loop;
    }
  }

  void setSocketDescriptor(u32 socketdescriptor) { this->socketdescriptor = socketdescriptor; }

  /** Sets time out duration in millisecond.
   * This method can be called only after a connection is opened.
   * @param[in] durationInMilliSec time out duration in ms
   */
  void setTimeout(f64 durationInMilliSec) {
    if (socketdescriptor != 0) {
      timeoutDurationInMilliSec = durationInMilliSec;
      struct timeval tv;
      tv.tv_sec = static_cast<u32>(std::floor(durationInMilliSec / 1000.));
      if (durationInMilliSec > std::floor(durationInMilliSec)) {
        tv.tv_usec = (i32)((durationInMilliSec - std::floor(durationInMilliSec)) * 1000);
      } else {
        tv.tv_usec = (i32)(durationInMilliSec * 1000);
      }
      setsockopt(socketdescriptor, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof tv);
    } else {
      throw TCPSocketException(TCPSocketException::TimeoutDurationCannotBeSetToDisconnectedSocket);
    }
  }

  void setNoDelay() {
    const i32 flag = 1;
    setsockopt(socketdescriptor, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&flag), sizeof(i32));
  }

  f64 getTimeoutDuration() const { return timeoutDurationInMilliSec; }
  u16 getPort() const { return port; }
  void setPort(u16 port) { this->port = port; }
  TCPSocket::State getStatus() const { return status; }
  bool isConnected() const { return status == State::TCPSocketConnected; }

 protected:
  State status{State::TCPSocketInitialized};
  i32 socketdescriptor{};
  i32 port{};

 private:
  f64 timeoutDurationInMilliSec{};
};

/** A class that represents a socket which is created by a server
 * when a new client is connected. This is used to perform multi-client
 * data transfer with TCPSocket server socket.
 */
class TCPServerAcceptedSocket : public TCPSocket {
 public:
  /** Constructs an instance.
   */
  TCPServerAcceptedSocket() {
#ifdef SO_NOSIGPIPE
    const i32 n = 1;
    ::setsockopt(this->socketdescriptor, SOL_SOCKET, SO_NOSIGPIPE, &n, sizeof(n));
#endif
  }

  ~TCPServerAcceptedSocket() override = default;

  /** Clones an instance.
   * This method is just for debugging purposes and not for ordinary user application.
   */
  void close() {
    switch (status) {
      case State::TCPSocketCreated:
      case State::TCPSocketBound:
      case State::TCPSocketListening:
      case State::TCPSocketConnected:
        ::close(socketdescriptor);
        break;
      default:
        break;
    }
    status = State::TCPSocketInitialized;
  }

  /** Not for ordinary user application.
   */
  void setAddress(struct ::sockaddr_in* address) { memcpy(&address, address, sizeof(struct ::sockaddr_in)); }
};

/** A class that represents a TCP server socket.
 */
class TCPServerSocket : public TCPSocket {
 public:
  TCPServerSocket(i32 portNumber) : TCPSocket(portNumber) {}

  ~TCPServerSocket() override = default;

  void open() {
    if (getPort() < 0) {
      throw TCPSocketException(TCPSocketException::PortNumberError);
    }
    create();
    bind();
    listen();
    const i32 n = 1;
#ifdef SO_NOSIGPIPE
    ::setsockopt(this->socketdescriptor, SOL_SOCKET, SO_NOSIGPIPE, &n, sizeof(n));
#endif
  }

  void close() {
    switch (status) {
      case State::TCPSocketCreated:
      case State::TCPSocketBound:
      case State::TCPSocketListening:
      case State::TCPSocketConnected:
        ::close(socketdescriptor);
        break;
      default:
        break;
    }
    status = State::TCPSocketInitialized;
  }

  TCPServerAcceptedSocket* accept() {
    struct ::sockaddr_in clientaddress;
    const auto length = sizeof(clientaddress);
    const auto result = ::accept(socketdescriptor, (struct ::sockaddr*)&clientaddress, (::socklen_t*)&length);
    if (result < 0) {
      throw TCPSocketException(TCPSocketException::AcceptException);
    } else {
      TCPServerAcceptedSocket* acceptedsocket = new TCPServerAcceptedSocket();
      acceptedsocket->setAddress(&clientaddress);
      acceptedsocket->setPort(getPort());
      acceptedsocket->setSocketDescriptor(result);
      return acceptedsocket;
    }
  }

  TCPServerAcceptedSocket* accept(double timeoutDurationInMilliSec) {
    fd_set mask;
    FD_ZERO(&mask);
    FD_SET(socketdescriptor, &mask);
    struct timeval tv;
    tv.tv_sec = (unsigned int)(floor(timeoutDurationInMilliSec / 1000.));
    tv.tv_usec = (int)(timeoutDurationInMilliSec * 1000 /* us */
                       - ((int)((timeoutDurationInMilliSec * 1000)) / 10000));

    auto result = select(socketdescriptor + 1, &mask, NULL, NULL, &tv);
    if (result < 0) {
      throw TCPSocketException(TCPSocketException::AcceptException);
    }
    if (result == 0) {
      throw TCPSocketException(TCPSocketException::Timeout);
    }

    struct ::sockaddr_in clientaddress;

    const auto length = sizeof(clientaddress);
    result = 0;
    result = ::accept(socketdescriptor, (struct ::sockaddr*)&clientaddress, (::socklen_t*)&length);

    if (result < 0) {
      throw TCPSocketException(TCPSocketException::AcceptException);
    } else {
      TCPServerAcceptedSocket* acceptedsocket = new TCPServerAcceptedSocket();
      acceptedsocket->setAddress(&clientaddress);
      acceptedsocket->setPort(getPort());
      acceptedsocket->setSocketDescriptor(result);
      return acceptedsocket;
    }
  }

 private:
  void create() {
    const auto result = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (result < 0) {
      throw TCPSocketException(TCPSocketException::TCPSocketError);
    } else {
      status = State::TCPSocketCreated;
      socketdescriptor = result;
    }
  }

  void bind() {
    struct ::sockaddr_in serveraddress;
    memset(&serveraddress, 0, sizeof(struct ::sockaddr_in));
    serveraddress.sin_family = AF_INET;
    serveraddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddress.sin_port = htons(getPort());
    const i32 yes = 1;
    setsockopt(socketdescriptor, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes));
    const auto result = ::bind(socketdescriptor, (struct ::sockaddr*)&serveraddress, sizeof(struct ::sockaddr_in));
    if (result < 0) {
      throw TCPSocketException(TCPSocketException::BindError);
    } else {
      status = State::TCPSocketBound;
    }
  }

  void listen() {
    const i32 result = ::listen(socketdescriptor, maxofconnections);
    if (result < 0) {
      throw TCPSocketException(TCPSocketException::ListenError);
    } else {
      status = State::TCPSocketListening;
    }
  }

  static const i32 maxofconnections = 5;
};

class TCPClientSocket : public TCPSocket {
 public:
  TCPClientSocket(std::string url, i32 port) : TCPSocket(port), url(url) {}
  ~TCPClientSocket() override = default;

  void open(double timeoutDurationInMilliSec = 1000) {
    if (url.size() == 0) {
      throw TCPSocketException(TCPSocketException::OpenException);
    }
    create();
    connect(timeoutDurationInMilliSec);
  }

  void open(const std::string& url, i32 port, double timeoutDurationInMilliSec = 1000) {
    open(timeoutDurationInMilliSec);
    const i32 n = 1;
#ifdef SO_NOSIGPIPE
    ::setsockopt(this->socketdescriptor, SOL_SOCKET, SO_NOSIGPIPE, &n, sizeof(n));
#endif
  }

  void close() {
    switch (status) {
      case State::TCPSocketCreated:
      case State::TCPSocketBound:
      case State::TCPSocketListening:
      case State::TCPSocketConnected:
        ::close(socketdescriptor);
        break;
      default:
        break;
    }
    status = State::TCPSocketInitialized;
  }

 private:
  void create() {
    int result = ::socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (result < 0) {
      throw TCPSocketException(TCPSocketException::CreateException);
    } else {
      status = State::TCPSocketCreated;
      socketdescriptor = result;
    }
  }

  void connect(double timeoutDurationInMilliSec) {
    using namespace std;
    i32 result;
    if (status == State::TCPSocketConnected) {
      return;
    }
    struct ::sockaddr_in serveraddress;
    memset(&serveraddress, 0, sizeof(struct ::sockaddr_in));
    serveraddress.sin_family = AF_INET;
    serveraddress.sin_port = htons(getPort());
    // set url or ip address
    struct ::hostent* hostentry;
    hostentry = ::gethostbyname(url.c_str());
    if (hostentry == NULL) {
      throw TCPSocketException(TCPSocketException::HostEntryError);
    } else {
      serveraddress.sin_addr.s_addr = *((unsigned long*)hostentry->h_addr_list[0]);
    }

    result = 0;
    i32 flag = fcntl(socketdescriptor, F_GETFL, 0);
    if (flag < 0) {
      throw TCPSocketException(TCPSocketException::ConnectExceptionWhenChangingSocketModeToNonBlocking);
    }
    if (fcntl(socketdescriptor, F_SETFL, flag | O_NONBLOCK) < 0) {
      throw TCPSocketException(TCPSocketException::ConnectExceptionWhenChangingSocketModeToNonBlocking);
    }

    result = ::connect(socketdescriptor, (struct ::sockaddr*)&serveraddress, sizeof(struct ::sockaddr_in));

    if (result < 0) {
      if (errno == EINPROGRESS) {
        struct timeval tv;
        tv.tv_sec = (unsigned int)(floor(timeoutDurationInMilliSec / 1000.));
        tv.tv_usec = (int)((timeoutDurationInMilliSec - floor(timeoutDurationInMilliSec)) * 1000);
        fd_set rmask, wmask;
        FD_ZERO(&rmask);
        FD_ZERO(&wmask);
        FD_SET(socketdescriptor, &wmask);
        result = select(socketdescriptor + 1, NULL, &wmask, NULL, &tv);
        if (result < 0) {
          throw TCPSocketException(TCPSocketException::ConnectExceptionWhenWaitingForConnection);
        } else if (result == 0) {
          // timeout happened
          throw TCPSocketException(TCPSocketException::Timeout);
        } else {
          struct sockaddr_in name;
          socklen_t len = sizeof(name);
          if (getpeername(socketdescriptor, (struct sockaddr*)&name, &len) >= 0) {
            // connected
            status = State::TCPSocketConnected;
            // reset flag
            if (fcntl(socketdescriptor, F_SETFL, flag) < 0) {
              throw TCPSocketException(TCPSocketException::ConnectExceptionWhenChangingSocketModeToBlocking);
            }
            return;
          } else {
            throw TCPSocketException(TCPSocketException::ConnectFailed);
          }
        }
      } else {
        throw TCPSocketException(TCPSocketException::ConnectExceptionNonBlockingConnectionReturnedUnexpecedResult);
      }
    } else {
      throw TCPSocketException(TCPSocketException::ConnectExceptionNonBlockingConnectionImmediateluSucceeded);
    }
  }

  std::string url;
};

#endif /* SPACEWIRE_TCPSOCKET_HH_ */
