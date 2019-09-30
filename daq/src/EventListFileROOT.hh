#ifndef EVENTLISTFILEROOT_HH_
#define EVENTLISTFILEROOT_HH_

#include "CxxUtilities/ROOTUtilities.hh"
#include "EventListFile.hh"
#include "TCanvas.h"
#include "TFile.h"
#include "TGraph.h"
#include "TH1D.h"
#include "TTree.h"

class EventListFileROOT : public EventListFile {
 public:
  EventListFileROOT(std::string fileName, std::string detectorID = "empty", std::string configurationYAMLFile = "");
  ~EventListFileROOT();
  void fillEvents(std::vector<GROWTH_FY2015_ADC_Type::Event*>& events);
  size_t getEntries();
  void fillGPSTime(uint8_t* gpsTimeRegisterBuffer);
  void close();

 private:
  void createOutputRootFile();
  void copyEventData(GROWTH_FY2015_ADC_Type::Event* from, GROWTH_FY2015_ADC_Type::Event* to);
  void writeHeader();

 private:
  std::unique_ptr<TTree> eventTree;
  std::unique_ptr<TFile> outputFile;
  std::string detectorID{};
  uint32_t unixTime{};
  GROWTH_FY2015_ADC_Type::Event eventEntry{};
  std::string configurationYAMLFile{};
};

#endif /* EVENTLISTFILEROOT_HH_ */
