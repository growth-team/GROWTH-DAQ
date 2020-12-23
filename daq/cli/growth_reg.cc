#include "registeraccessautomator.hh"

#include "spdlog/spdlog.h"

#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
  // TODO: use CLI argument parsing library
  if (argc < 3) {
    using namespace std;
    cout << "Provide UART device name (e.g. /dev/tty.usb-aaa-bbb-port0) and register-setting YAML file." << endl;
    cout << endl;
    ::exit(-1);
  }
  const std::string deviceName(argv[1]);
  const std::string yamlFile(argv[2]);

  RegisterAccessAutomator registerAccessAutomator(deviceName);
  registerAccessAutomator.run(yamlFile);

  return 0;
}
