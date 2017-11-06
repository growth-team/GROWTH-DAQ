#include "CxxUtilities/CxxUtilities.hh"
#include "SerialPort.hh"

class ReceiveThread : public CxxUtilities::StoppableThread {
 private:
  SerialPort *port;

 public:
  ReceiveThread(SerialPort *port) { this->port = port; }

 public:
  void run() {
    using namespace std;
    cerr << "ReceiveThread started." << endl;
    // std::vector<uint8_t> receiveBuffer(1024);
    uint8_t receiveBuffer[128];
    size_t nReceivedBytes;
    while (!stopped) {
      try {
        cout << "Receive() loop:" << endl;
        // port->receiveWithTimeout(boost::asio::buffer(receiveBuffer), nReceivedBytes, boost::posix_time::seconds(2));
        nReceivedBytes = port->receive(receiveBuffer, 128);
      } catch (boost::system::system_error &e) {
        using namespace std;
        cerr << e.code() << "," << e.what() << endl;
        continue;
      } catch (SerialPortException &e) {
        cout << "SerialPortException " << e.toString() << endl;
        ::exit(-1);
      }
      size_t length = nReceivedBytes;
      cout << "Received " << nReceivedBytes << " bytes" << endl;
      for (size_t i = 0; i < length; i++) {
        cout << "Received" << dec << i << ": "
             << "0x" << hex << right << setw(2) << setfill('0') << (uint32_t)receiveBuffer[i] << endl;
      }
    }
  }
};

//#include "SpaceWireSSDTPModuleUART.hh"
int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "Provide serial port device (e.g. /dev/tty.usb-xxxxx)." << std::endl;
    return 1;
  }

  SerialPort serialPort(argv[1], 230400);
  ReceiveThread receiveThread(&serialPort);
  receiveThread.start();

  std::vector<uint8_t> sendData = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0xfe, 0x01,
                                   0x4c, 0x00, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x12, 0x00, 0x00, 0x02, 0xbe};

  serialPort.send(sendData);

  for (size_t i = 0; i < 10; i++) {
    serialPort.send(sendData);
    sendData.push_back(i);
  }

  receiveThread.join();
  return 0;
}
