#ifndef GROWTHDAQ_EVENTLISTFILE_HH_
#define GROWTHDAQ_EVENTLISTFILE_HH_

#include "types.h"
#include <string>
#include <vector>

#include "growth-fpga/types.hh"

/// Represents an event list file.
class EventListFile {
 public:
  EventListFile(std::string fileName) : fileName_(std::move(fileName)) {}
  virtual ~EventListFile() = default;
  virtual void fillEvents(const EventList& events) = 0;
  virtual void fillGPSTime(
      const std::array<u8, GROWTH_FY2015_ADC::GPS_TIME_REG_SIZE_BYTES + 1>& gpsTimeRegisterBuffer) = 0;
  virtual size_t getEntries() const = 0;
  virtual void close() = 0;
  const std::string& fileName() const { return fileName_; }

 protected:
  std::string fileName_{};
};

#endif
