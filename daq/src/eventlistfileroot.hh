#ifndef GROWTHDAQ_EVENTLISTFILEROOT_HH_
#define GROWTHDAQ_EVENTLISTFILEROOT_HH_

#include "types.h"
#include "rootutil.hh"
#include "EventListFile.hh"
#include "TCanvas.h"
#include "TFile.h"
#include "TTree.h"

class EventListFileROOT : public EventListFile {
 public:
  EventListFileROOT(std::string fileName, std::string detectorID = "empty", std::string configurationYAMLFile = "");
  ~EventListFileROOT();
  void fillGPSTime(const u8* gpsTimeRegisterBuffer) override;
  void fillEvents(const std::vector<growth_fpga::Event*>& events) override;
  size_t getEntries() const override;
  void close() override;

 private:
  void createOutputRootFile();
  void copyEventData(growth_fpga::Event* from, growth_fpga::Event* to);
  void writeHeader();

 private:
  std::unique_ptr<TTree> eventTree_;
  std::unique_ptr<TFile> outputFile_;
  std::string detectorID_{};
  u32 unixTime_{};
  growth_fpga::Event eventEntry_{};
  std::string configurationYAMLFile_{};
};

#endif /* EVENTLISTFILEROOT_HH_ */
