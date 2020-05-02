#ifndef EVENTLISTFILEROOT_HH_
#define EVENTLISTFILEROOT_HH_

#include "CxxUtilities/ROOTUtilities.hh"
#include "EventListFile.hh"
#include "TCanvas.h"
#include "TFile.h"
#include "TTree.h"

class EventListFileROOT : public EventListFile {
 public:
  EventListFileROOT(std::string fileName, std::string detectorID = "empty", std::string configurationYAMLFile = "");
  ~EventListFileROOT();
  void fillEvents(std::vector<GROWTH_FY2015_ADC_Type::Event*>& events);
  size_t getEntries() const;
  void fillGPSTime(uint8_t* gpsTimeRegisterBuffer);
  void close();

 private:
  void createOutputRootFile();
  void copyEventData(GROWTH_FY2015_ADC_Type::Event* from, GROWTH_FY2015_ADC_Type::Event* to);
  void writeHeader();

 private:
  std::unique_ptr<TTree> eventTree_;
  std::unique_ptr<TFile> outputFile_;
  std::string detectorID_{};
  uint32_t unixTime_{};
  GROWTH_FY2015_ADC_Type::Event eventEntry_{};
  std::string configurationYAMLFile_{};
};

#endif /* EVENTLISTFILEROOT_HH_ */
