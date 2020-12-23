#include "../serialport.hh"
#include "CxxUtilities/CxxUtilities.hh"

class ReceiveThread : public CxxUtilities::StoppableThread {
 private:
  SerialPort* port;

 public:
  ReceiveThread(SerialPort* port) { this->port = port; }

 public:
  void run() {
    using namespace std;
    cerr << "ReceiveThread started." << endl;
    // std::vector<u8> receiveBuffer(1024);
    u8 receiveBuffer[128];
    size_t nReceivedBytes;
    port->setTimeout(10000);  // ms

    while (!stopped) {
      try {
        cout << "Receive() loop:" << endl;
        // port->receiveWithTimeout(boost::asio::buffer(receiveBuffer), nReceivedBytes, boost::posix_time::seconds(2));
        nReceivedBytes = port->receive(receiveBuffer, 128);
      } catch (boost::system::system_error& e) {
        using namespace std;
        cerr << e.code() << "," << e.what() << endl;
        continue;
      } catch (SerialPortException& e) {
        cout << "SerialPortException " << e.toString() << endl;
        ::exit(-1);
      }
      size_t length = nReceivedBytes;
      cout << "Received " << dec << nReceivedBytes << " bytes" << endl;
      for (size_t i = 0; i < length; i++) {
        cout << "Received" << dec << i << ": "
             << "0x" << hex << right << setw(2) << setfill('0') << (u32)receiveBuffer[i];
        if (0x41 <= receiveBuffer[i] && receiveBuffer[i] <= 0x7A) {
          cout << "(" << receiveBuffer[i] << ")";
        }
        cout << endl;
      }
    }
  }
};

//#include "SpaceWireSSDTPModuleUART.hh"
int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cerr << "Provide serial port device (e.g. /dev/tty.usb-xxxxx)." << std::endl;
    return 1;
  }

  SerialPort serialPort(argv[1], 230400);
  ReceiveThread receiveThread(&serialPort);
  receiveThread.start();

  receiveThread.join();
  return 0;
}
