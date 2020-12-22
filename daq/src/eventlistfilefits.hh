#ifndef GROWTHDAQ_EVENTLISTFILEFITS_HH_
#define GROWTHDAQ_EVENTLISTFILEFITS_HH_

extern "C" {
#include "fitsio.h"
}

#include "adcboard.hh"
#include "eventlistfile.hh"
#include "fitsutil.hh"
#include "types.h"

#include <mutex>


class EventListFileFITS : public EventListFile {
 public:
  EventListFileFITS(const std::string& fileName, const std::string& detectorID = "empty",
                    const std::string& configurationYAMLFile = "", size_t nSamples = 1024, f64 exposureInSec = 0.0,  //
                    u32 fpgaType = 0x00000000, u32 fpgaVersion = 0x00000000);
  ~EventListFileFITS();

  /** Fill an entry to the HDU containing GPS Time and FPGA Time Tag.
   * Length of the data should be the same as GROWTH_FY2015_ADC::LengthOfGPSTimeRegister.
   * @param[in] buffer buffer containing a GPS Time Register data
   */
  void fillEvents(const std::vector<GROWTH_FY2015_ADC_Type::Event*>& events) override;
  void fillGPSTime(const std::array<u8, GROWTH_FY2015_ADC::GPS_TIME_REG_SIZE_BYTES + 1>& gpsTimeRegisterBuffer) override;
  size_t getEntries() const override { return rowIndexEvent_; }
  void close() override;
  void expandIfNecessary();

 private:
  void createOutputFITSFile();
  void writeHeader();
  void reportErrorThenQuitIfError(int fitsStatus, const std::string& methodName);

  fitsfile* outputFile_{};
  const std::string detectorID_{};
  const std::string configurationYAMLFile_{};

  static constexpr size_t INITIAL_ROW_NUMBER_EVENTS = 100000;
  static constexpr size_t INITIAL_ROW_NUMBER_GPS = 1000;
  i32 fitsStatus_ = 0;
  bool outputFileIsOpen_ = false;
  size_t rowIndexEvent_{};   // will be initialized in createOutputFITSFile()
  size_t rowIndexGPS_{};     // will be initialized in createOutputFITSFile()
  size_t fitsNRowsEvent_{};  // currently allocated rows
  size_t rowExpansionStep = INITIAL_ROW_NUMBER_EVENTS;
  const size_t nWaveformSamples_{};
  const f64 exposureInSec_{};
  const u32 fpgaType_{};
  const u32 fpgaVersion_{};

  std::mutex fitsAccessMutex_;

  const i32 FIRST_ELEMENT = 1;

  //---------------------------------------------
  // event list HDU
  //---------------------------------------------
  static const size_t NUM_COLUMNS_EVENT_HDU = 11;
  const char* ttypes[NUM_COLUMNS_EVENT_HDU] = {
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
  const size_t MAX_TFORM = 1024;
  char* tforms[NUM_COLUMNS_EVENT_HDU] = {
      //
      const_cast<char*>("B") /*u8*/,                // boardIndexAndChannel
      const_cast<char*>("K") /*u64*/,               // timeTag
      const_cast<char*>("U") /*u16*/,               // triggerCount
      const_cast<char*>("U") /*u16*/,               // phaMax
      const_cast<char*>("U") /*u16*/,               // phaMaxTime
      const_cast<char*>("U") /*u16*/,               // phaMin
      const_cast<char*>("U") /*u16*/,               // phaFirst
      const_cast<char*>("U") /*u16*/,               // phaLast
      const_cast<char*>("U") /*u16*/,               // maxDerivative
      const_cast<char*>("U") /*u16*/,               // baseline
      new char[MAX_TFORM] /* to be filled later */  //
  };
  const char* tunits[NUM_COLUMNS_EVENT_HDU] = {
      //
      "", "", "", "", "", ""  //
  };
  enum ColumnIndexEvent {
    COL_CHANNEL = 1,
    COL_TIMETAG = 2,
    COL_TRIGGER_COUNT = 3,
    COL_PHA_MAX = 4,
    COL_PHA_MAX_TIME = 5,
    COL_PHA_MIN = 6,
    COL_PHA_FIRST = 7,
    COL_PHA_LAST = 8,
    COL_MAX_DERIVATIVE = 9,
    COL_BASELINE = 10,
    COL_WAVEFORM = 11
  };

  //---------------------------------------------
  // GPS Time Register HDU
  //---------------------------------------------
  static constexpr size_t NUM_COLUMNS_GPS_HDU = 3;
  const char* ttypesGPS[NUM_COLUMNS_GPS_HDU] = {
      //
      "fpgaTimeTag",  //
      "unixTime",     //
      "gpsTime"       //
  };
  const char* tformsGPS[NUM_COLUMNS_GPS_HDU] = {
      //
      "K",   //
      "V",   //
      "14A"  //
  };
  const char* tunitsGPS[NUM_COLUMNS_GPS_HDU] = {
      //
      "",  //
      "",  //
      ""   //
  };
  enum ColumnIndexGPS {
    COL_FPGA_TIMETAG = 1,  //
    COL_UNIXTIME = 2,      //
    COL_GPSTIME = 3
  };
};

#endif
