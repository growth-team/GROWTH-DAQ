#ifndef EVENTLISTFILEFITS_HH_
#define EVENTLISTFILEFITS_HH_

extern "C" {
#include "fitsio.h"
}

#include "CxxUtilities/FitsUtility.hh"
#include "EventListFile.hh"
#include "adcboard.hh"
#include "types.h"

#include <mutex>

class EventListFileFITS : public EventListFile {
 public:
  EventListFileFITS(const std::string& fileName, const std::string& detectorID = "empty",
                    const std::string& configurationYAMLFile = "", size_t nSamples = 1024, f64 exposureInSec = 0,  //
                    u32 fpgaType = 0x00000000, u32 fpgaVersion = 0x00000000);
  ~EventListFileFITS();

  /** Fill an entry to the HDU containing GPS Time and FPGA Time Tag.
   * Length of the data should be the same as GROWTH_FY2015_ADC::LengthOfGPSTimeRegister.
   * @param[in] buffer buffer containing a GPS Time Register data
   */
  void fillEvents(const std::vector<GROWTH_FY2015_ADC_Type::Event*>& events) override;
  void fillGPSTime(const u8* gpsTimeRegisterBuffer) override;
  size_t getEntries() const override { return rowIndex; }
  void close() override;
  void expandIfNecessary();

 private:
  void createOutputFITSFile();
  void writeHeader();
  void reportErrorThenQuitIfError(int fitsStatus, const std::string& methodName);

  fitsfile* outputFile{};
  std::string detectorID{};
  std::string configurationYAMLFile{};

  static constexpr size_t InitialRowNumberEvent = 1000;
  static constexpr size_t InitialRowNumberGPS = 1000;
  i32 fitsStatus = 0;
  bool outputFileIsOpen = false;
  size_t rowIndex{};     // will be initialized in createOutputFITSFile()
  size_t rowIndexGPS{};  // will be initialized in createOutputFITSFile()
  size_t fitsNRows{};    // currently allocated rows
  size_t rowExpansionStep = InitialRowNumberEvent;
  size_t nSamples{};
  f64 exposureInSec{};
  u32 fpgaType = 0x00000000;
  u32 fpgaVersion = 0x00000000;

  std::mutex fitsAccessMutex_;

  const i32 firstElement = 1;

  //---------------------------------------------
  // event list HDU
  //---------------------------------------------
  static const size_t NumColumnsEvent = 11;
  const char* ttypes[NumColumnsEvent] = {
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
  const size_t MaxTFORM = 1024;
  char* tforms[NumColumnsEvent] = {
      //
      const_cast<char*>("B") /*u8*/,               // boardIndexAndChannel
      const_cast<char*>("K") /*u64*/,              // timeTag
      const_cast<char*>("U") /*u16*/,              // triggerCount
      const_cast<char*>("U") /*u16*/,              // phaMax
      const_cast<char*>("U") /*u16*/,              // phaMaxTime
      const_cast<char*>("U") /*u16*/,              // phaMin
      const_cast<char*>("U") /*u16*/,              // phaFirst
      const_cast<char*>("U") /*u16*/,              // phaLast
      const_cast<char*>("U") /*u16*/,              // maxDerivative
      const_cast<char*>("U") /*u16*/,              // baseline
      new char[MaxTFORM] /* to be filled later */  //
  };
  const char* tunits[NumColumnsEvent] = {
      //
      "", "", "", "", "", ""  //
  };
  enum ColumnIndexEvent {
    Column_boardIndexAndChannel = 1,
    Column_timeTag = 2,
    Column_triggerCount = 3,
    Column_phaMax = 4,
    Column_phaMaxTime = 5,
    Column_phaMin = 6,
    Column_phaFirst = 7,
    Column_phaLast = 8,
    Column_maxDerivative = 9,
    Column_baseline = 10,
    Column_waveform = 11
  };

  //---------------------------------------------
  // GPS Time Register HDU
  //---------------------------------------------
  static constexpr size_t NumColumnsGPS = 3;
  const char* ttypesGPS[NumColumnsGPS] = {
      //
      "fpgaTimeTag",  //
      "unixTime",     //
      "gpsTime"       //
  };
  const char* tformsGPS[NumColumnsGPS] = {
      //
      "K",   //
      "V",   //
      "14A"  //
  };
  const char* tunitsGPS[NumColumnsGPS] = {
      //
      "",  //
      "",  //
      ""   //
  };
  enum ColumnIndexGPS {
    Column_fpgaTimeTag = 1,  //
    Column_unixTime = 2,     //
    Column_gpsTime = 3
  };
};

#endif /* EVENTLISTFILEFITS_HH_ */
