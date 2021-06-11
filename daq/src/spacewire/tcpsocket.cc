#include "spacewire/tcpsocket.hh"

#include <cstring>
#include <cmath>
#include <cassert>

TCPSocket::TCPSocket(i16 port) : port(port) {}

size_t TCPSocket::send(const u8* data, size_t length) {
  const i32 result = ::send(socketdescriptor, reinterpret_cast<const void*>(data), length, 0);
  if (result < 0) {
    throw TCPSocketException(TCPSocketException::TCPSocketError);
  }
  return result;
}

size_t TCPSocket::receiveLoopUntilSpecifiedLengthCompletes(u8* data, size_t length) {
  return receive(data, length, true);
}

size_t TCPSocket::receive(u8* data, size_t length, bool waitUntilSpecifiedLengthCompletes) {
  i32 result = 0;
  size_t remainingLength = length;
  i32 readDoneLength = 0;

_tcpocket_receive_loop:  //
  result = ::recv(socketdescriptor, reinterpret_cast<void*>(data + readDoneLength), remainingLength, 0);

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
    assert(remainingLength >= result);
    remainingLength = remainingLength - result;
    readDoneLength = readDoneLength + result;
    if (remainingLength == 0) {
      return length;
    }
    goto _tcpocket_receive_loop;
  }
}

/** Sets time out duration in millisecond.
 * This method can be called only after a connection is opened.
 * @param[in] durationInMilliSec time out duration in ms
 */
void TCPSocket::setTimeout(f64 durationInMilliSec) {
  if (socketdescriptor != 0) {
    timeoutDurationInMilliSec_ = durationInMilliSec;
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

void TCPSocket::setNoDelay() {
  const i32 flag = 1;
  setsockopt(socketdescriptor, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&flag), sizeof(i32));
}

TCPServerAcceptedSocket::TCPServerAcceptedSocket() {
#ifdef SO_NOSIGPIPE
  const i32 n = 1;
  ::setsockopt(this->socketdescriptor, SOL_SOCKET, SO_NOSIGPIPE, &n, sizeof(n));
#endif
}

/** Clones an instance.
 * This method is just for debugging purposes and not for ordinary user application.
 */
void TCPServerAcceptedSocket::close() {
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

 void TCPServerAcceptedSocket::setAddress(const sockaddr_in& address) {
   clientAddress_ = address;
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

TCPServerSocket::TCPServerSocket(i32 portNumber) : TCPSocket(portNumber) {}

void TCPServerSocket::open() {
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

void TCPServerSocket::close() {
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

TCPServerAcceptedSocket* TCPServerSocket::accept() {
  struct ::sockaddr_in clientaddress;
  const auto length = sizeof(clientaddress);
  const auto result = ::accept(socketdescriptor, (struct ::sockaddr*)&clientaddress, (::socklen_t*)&length);
  if (result < 0) {
    throw TCPSocketException(TCPSocketException::AcceptException);
  } else {
    TCPServerAcceptedSocket* acceptedsocket = new TCPServerAcceptedSocket();
    acceptedsocket->setAddress(clientaddress);
    acceptedsocket->setPort(getPort());
    acceptedsocket->setSocketDescriptor(result);
    return acceptedsocket;
  }
}

TCPServerAcceptedSocket* TCPServerSocket::accept(f64 timeoutDurationInMilliSec) {
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
    acceptedsocket->setAddress(clientaddress);
    acceptedsocket->setPort(getPort());
    acceptedsocket->setSocketDescriptor(result);
    return acceptedsocket;
  }
}

void TCPServerSocket::create() {
  const auto result = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (result < 0) {
    throw TCPSocketException(TCPSocketException::TCPSocketError);
  } else {
    status = State::TCPSocketCreated;
    socketdescriptor = result;
  }
}

void TCPServerSocket::bind() {
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

void TCPServerSocket::listen() {
  const i32 result = ::listen(socketdescriptor, maxofconnections);
  if (result < 0) {
    throw TCPSocketException(TCPSocketException::ListenError);
  } else {
    status = State::TCPSocketListening;
  }
}

TCPClientSocket::TCPClientSocket(std::string url, i32 port) : TCPSocket(port), url(url) {}

void TCPClientSocket::open(f64 timeoutDurationInMilliSec) {
  if (url.empty()) {
    throw TCPSocketException(TCPSocketException::OpenException);
  }
  create();
  connect(timeoutDurationInMilliSec);
}

void TCPClientSocket::close() {
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

void TCPClientSocket::create() {
  int result = ::socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (result < 0) {
    throw TCPSocketException(TCPSocketException::CreateException);
  } else {
    status = State::TCPSocketCreated;
    socketdescriptor = result;
  }
}

void TCPClientSocket::connect(f64 timeoutDurationInMilliSec) {
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
