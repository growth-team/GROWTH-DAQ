#ifndef EVENTLISTFILEFITS_HH_
#define EVENTLISTFILEFITS_HH_

extern "C" {
#include "fitsio.h"
}

#include "CxxUtilities/FitsUtility.hh"
#include "EventListFile.hh"
#include "adcboard.hh"

class EventListFileFITS : public EventListFile {

 public:
  EventListFileFITS(std::string fileName, std::string detectorID = "empty", std::string configurationYAMLFile = "",
                    size_t nSamples = 1024, double exposureInSec = 0,  //
                    uint32_t fpgaType = 0x00000000, uint32_t fpgaVersion = 0x00000000);
  ~EventListFileFITS();

  /** Fill an entry to the HDU containing GPS Time and FPGA Time Tag.
   * Length of the data should be the same as GROWTH_FY2015_ADC::LengthOfGPSTimeRegister.
   * @param[in] buffer buffer containing a GPS Time Register data
   */
  void fillGPSTime(uint8_t* gpsTimeRegisterBuffer);
  void fillEvents(std::vector<GROWTH_FY2015_ADC_Type::Event*>& events);
  void expandIfNecessary();
  size_t getEntries();
  void close();

 private:
  void createOutputFITSFile();
  void writeHeader();
  void reportErrorThenQuitIfError(int fitsStatus, std::string methodName);

 private:
  fitsfile* outputFile{};
  std::string detectorID{};
  GROWTH_FY2015_ADC_Type::Event eventEntry{};
  std::string configurationYAMLFile{};

  static const size_t InitialRowNumber     = 1000;
  static const size_t InitialRowNumber_GPS = 1000;
  int fitsStatus                           = 0;
  bool outputFileIsOpen                    = false;
  size_t rowIndex{};      // will be initialized in createOutputFITSFile()
  size_t rowIndex_GPS{};  // will be initialized in createOutputFITSFile()
  size_t fitsNRows{};     // currently allocated rows
  size_t rowExpansionStep = InitialRowNumber;
  size_t nSamples{};
  double exposureInSec{};
  uint32_t fpgaType    = 0x00000000;
  uint32_t fpgaVersion = 0x00000000;

  CxxUtilities::Mutex fitsAccessMutes{};

  const int firstElement = 1;

  //---------------------------------------------
  // event list HDU
  //---------------------------------------------
  static const size_t nColumns_Event = 11;
  const char* ttypes[nColumns_Event]       = {
      //
      "boardIndexAndChannel",  // B
      "timeTag",               // K
      "triggerCount",          // U
      "phaMax",                // U
      "phaMaxTime",            // U
      "phaMin",                // U
      "phaFirst",              // U
      "phaLast",               // U
      "maxDerivative",         // U
      "baseline",              // U
      "waveform"               // B
  };
  const size_t MaxTFORM        = 1024;
  const char* tforms[nColumns_Event] = {
      //
      "B" /*uint8_t*/,                      // boardIndexAndChannel
      "K" /*uint64_t*/,                     // timeTag
      "U" /*uint16_t*/,                     // triggerCount
      "U" /*uint16_t*/,                     // phaMax
      "U" /*uint16_t*/,                     // phaMaxTime
      "U" /*uint16_t*/,                     // phaMin
      "U" /*uint16_t*/,                     // phaFirst
      "U" /*uint16_t*/,                     // phaLast
      "U" /*uint16_t*/,                     // maxDerivative
      "U" /*uint16_t*/,                     // baseline
      new char[MaxTFORM] /* to be filled later */  //
  };
  const char* tunits[nColumns_Event] = {
      //
      "", "", "", "", "", ""  //
  };
  enum columnIndices {
    Column_boardIndexAndChannel = 1,
    Column_timeTag              = 2,
    Column_triggerCount         = 3,
    Column_phaMax               = 4,
    Column_phaMaxTime           = 5,
    Column_phaMin               = 6,
    Column_phaFirst             = 7,
    Column_phaLast              = 8,
    Column_maxDerivative        = 9,
    Column_baseline             = 10,
    Column_waveform             = 11
  };

  //---------------------------------------------
  // GPS Time Register HDU
  //---------------------------------------------
  static const size_t nColumns_GPS = 3;
  const char* ttypes_GPS[nColumns_GPS]   = {
      //
      "fpgaTimeTag",  //
      "unixTime",     //
      "gpsTime"       //
  };
  const char* tforms_GPS[nColumns_GPS] = {
      //
      "K",   //
      "V",   //
      "14A"  //
  };
  const char* tunits_GPS[nColumns_GPS] = {
      //
      "",  //
      "",  //
      ""   //
  };
  enum columnIndices_GPS {
    Column_fpgaTimeTag = 1,  //
    Column_unixTime    = 2,  //
    Column_gpsTime     = 3
  };
};

#endif /* EVENTLISTFILEFITS_HH_ */
