#ifndef EVENTLISTFILEFITS_HH_
#define EVENTLISTFILEFITS_HH_

extern "C" {
#include "fitsio.h"
}

#include "CxxUtilities/FitsUtility.hh"
#include "EventListFile.hh"
#include "GROWTH_FY2015_ADC.hh"

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
  char* ttypes[nColumns_Event]       = {
      //
      (char*)"boardIndexAndChannel",  // B
      (char*)"timeTag",               // K
      (char*)"triggerCount",          // U
      (char*)"phaMax",                // U
      (char*)"phaMaxTime",            // U
      (char*)"phaMin",                // U
      (char*)"phaFirst",              // U
      (char*)"phaLast",               // U
      (char*)"maxDerivative",         // U
      (char*)"baseline",              // U
      (char*)"waveform"               // B
  };
  const size_t MaxTFORM        = 1024;
  char* tforms[nColumns_Event] = {
      //
      (char*)"B" /*uint8_t*/,                      // boardIndexAndChannel
      (char*)"K" /*uint64_t*/,                     // timeTag
      (char*)"U" /*uint16_t*/,                     // triggerCount
      (char*)"U" /*uint16_t*/,                     // phaMax
      (char*)"U" /*uint16_t*/,                     // phaMaxTime
      (char*)"U" /*uint16_t*/,                     // phaMin
      (char*)"U" /*uint16_t*/,                     // phaFirst
      (char*)"U" /*uint16_t*/,                     // phaLast
      (char*)"U" /*uint16_t*/,                     // maxDerivative
      (char*)"U" /*uint16_t*/,                     // baseline
      new char[MaxTFORM] /* to be filled later */  //
  };
  char* tunits[nColumns_Event] = {
      //
      (char*)"", (char*)"", (char*)"", (char*)"", (char*)"", (char*)""  //
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
  char* ttypes_GPS[nColumns_GPS]   = {
      //
      (char*)"fpgaTimeTag",  //
      (char*)"unixTime",     //
      (char*)"gpsTime"       //
  };
  char* tforms_GPS[nColumns_GPS] = {
      //
      (char*)"K",   //
      (char*)"V",   //
      (char*)"14A"  //
  };
  char* tunits_GPS[nColumns_GPS] = {
      //
      (char*)"",  //
      (char*)"",  //
      (char*)""   //
  };
  enum columnIndices_GPS {
    Column_fpgaTimeTag = 1,  //
    Column_unixTime    = 2,  //
    Column_gpsTime     = 3
  };
};

#endif /* EVENTLISTFILEFITS_HH_ */
