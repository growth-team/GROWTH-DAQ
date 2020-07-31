#ifndef EVENTLISTFILE_HH_
#define EVENTLISTFILE_HH_

#include "types.h"
#include "CxxUtilities/CxxUtilities.hh"
#include "GROWTH_FY2015_ADCModules/Types.hh"

/** Represents an event list file.
 */
class EventListFile {
 public:
  EventListFile(std::string fileName) : fileName_(std::move(fileName)) {}
  virtual ~EventListFile() = default;
  virtual void fillEvents(const std::vector<GROWTH_FY2015_ADC_Type::Event*>& events) = 0;
  virtual void fillGPSTime(const u8* gpsTimeRegisterBuffer) = 0;
  virtual size_t getEntries() const = 0;
  virtual void close() = 0;


 protected:
  std::string fileName_{};
};

#endif /* EVENTLISTFILE_HH_ */
