/*
 * test_serial_ssdtp.cc
 *
 *  Created on: Jun 9, 2015
 *      Author: yuasa
 */

#include "../serialport.hh"
#include "../spacewiressdtpmoduleuart.hh"
#include "CxxUtilities/CxxUtilities.hh"

class ReceiveThread : public CxxUtilities::StoppableThread {
 private:
  SpaceWireSSDTPModuleUART *ssdtp;

 public:
  ReceiveThread(SpaceWireSSDTPModuleUART *ssdtp) { this->ssdtp = ssdtp; }

 public:
  void run() {
    using namespace std;
    cerr << "ReceiveThread started." << endl;
    std::vector<u8> receivedData;

    while (!stopped) {
      try {
        cout << "Receive() loop:" << endl;
        receivedData = ssdtp->receive();
      } catch (SpaceWireSSDTPException &e) {
        cout << "SpaceWireSSDTPException " << e.toString() << endl;
        this->stop();
        return;
      }
      size_t length = receivedData.size();
      cout << "Received " << dec << length << " bytes" << endl;
      cout << "Dump ";
      for (size_t i = 0; i < length; i++) {
        cout << hex << right << setw(2) << setfill('0') << (u32)receivedData[i] << " ";
        if (i != 0) {
          if (receivedData[i - 1] + 1 != receivedData[i]) { cout << "Transfer Error! "; }
        }
      }
      cout << endl;
    }
  }
};

//#include "SpaceWireSSDTPModuleUART.hh"
int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "Provide serial port device (e.g. /dev/tty.usb-xxxxx)." << std::endl;
    return 1;
  }

  CxxUtilities::Condition c;

  SerialPort serialPort(argv[1], 230400);
  serialPort.setTimeout(10000);
  SpaceWireSSDTPModuleUART ssdtp(&serialPort);
  ReceiveThread receiveThread(&ssdtp);
  receiveThread.start();

  std::vector<u8> sendData;

  c.wait(100);
  for (size_t i = 0; i < 100; i++) {
    sendData.push_back(i);
    ssdtp.send(sendData);
  }

  receiveThread.join();
  return 0;
}
