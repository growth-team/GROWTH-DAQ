/*
 * test_growth_fy2015_adc_cpuTrigger.cc
 *
 *  Created on: Jun 10, 2015
 *      Author: yuasa
 */

#include "GROWTH_FY2015_ADC.hh"

int main(int argc, char* argv[]) {
  using namespace std;
  std::string deviceName = "/dev/null";
  auto adcBoard = new GROWTH_FY2015_ADC(deviceName);

  if (argc < 2) {
    cerr << "Provide configuration file." << endl;
    adcBoard->dumpMustExistKeywords();
    ::exit(-1);
  }

  std::string inputFileName(argv[1]);
  adcBoard->loadConfigurationFile(inputFileName);

  return 0;
}
