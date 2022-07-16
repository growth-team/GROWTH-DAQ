#include "eventlistfilefits.hh"

#include "stringutil.hh"
#include "timeutil.hh"
#include "spdlog/spdlog.h"

#include <fstream>
#include <cstring>
#include <chrono>

EventListFileFITS::EventListFileFITS(const std::string& fileName, const std::string& detectorID,
                                     const std::string& configurationYAMLFile, size_t nSamples, f64 exposureInSec,
                                     u32 fpgaType, u32 fpgaVersion)
    : EventListFile(fileName),
      detectorID_(std::move(detectorID)),
      nWaveformSamples_(nSamples),
      configurationYAMLFile_(std::move(configurationYAMLFile)),
      exposureInSec_(exposureInSec),
      fpgaType_(fpgaType),
      fpgaVersion_(fpgaVersion) {
  createOutputFITSFile();
}

EventListFileFITS::~EventListFileFITS() { close(); }

void EventListFileFITS::createOutputFITSFile() {
  std::lock_guard<std::mutex> guard(fitsAccessMutex_);

  rowIndexEvent_ = 0;
  rowIndexGPS_ = 0;

  size_t nColumns = NUM_COLUMNS_EVENT_HDU;

  if (nWaveformSamples_ == 0) {
    nColumns--;  // delete waveform column
  } else {
    std::stringstream ss;
    ss << nWaveformSamples_ << "U";
    strcpy(tforms[nColumns - 1], ss.str().c_str());
  }

  fitsNRowsEvent_ = INITIAL_ROW_NUMBER_EVENTS;
  const long naxis = 1;
  const long naxes[1] = {1};
  const i32 tbltype = BINARY_TBL;
  // Create FITS File
  if (fileName_[0] != '!') {
    fileName_ = "!" + fileName_;
  }
  fits_create_file(&outputFile_, fileName_.c_str(), &fitsStatus_);
  reportErrorThenQuitIfError(fitsStatus_, __func__);

  // Create FITS Image
  fits_create_img(outputFile_, USHORT_IMG, naxis, const_cast<long*>(naxes), &fitsStatus_);
  reportErrorThenQuitIfError(fitsStatus_, __func__);

  // Create BINTABLE
  fits_create_tbl(outputFile_, tbltype, fitsNRowsEvent_, nColumns, const_cast<char**>(ttypes), tforms,
                  const_cast<char**>(tunits), "EVENTS", &fitsStatus_);
  reportErrorThenQuitIfError(fitsStatus_, __func__);

  // write header info
  writeHeader();

  // Create GPS Time HDU
  fits_create_tbl(outputFile_, tbltype, INITIAL_ROW_NUMBER_GPS, NUM_COLUMNS_GPS_HDU, const_cast<char**>(ttypesGPS),
                  const_cast<char**>(tformsGPS), const_cast<char**>(tunitsGPS), "GPS", &fitsStatus_);
  reportErrorThenQuitIfError(fitsStatus_, __func__);

  fits_movnam_hdu(outputFile_, tbltype, const_cast<char*>("EVENTS"), 0, &fitsStatus_);
  reportErrorThenQuitIfError(fitsStatus_, __func__);

  outputFileIsOpen_ = true;
}

void EventListFileFITS::writeHeader() {
  /* sample code
   fits_update_key_str_check(fp, n=(char*)"TELESCOP", v->telescop, c->telescop, &istat)||
   fits_update_key_fixdbl(fp, n=(char*)"dec_pnt", v->dec_pnt, 8, c->dec_pnt, &istat)||
   fits_update_key_lng(fp, n=(char*)"EQUINOX", v->equinox, c->equinox, &istat)||
   *
   */

  char* n;  // keyword name
  const std::string fpgaTypeStr = stringutil::toHexString(fpgaType_, 8);
  const std::string fpgaVersionStr = stringutil::toHexString(fpgaVersion_, 8);
  const std::string creationDate = timeutil::getCurrentTimeYYYYMMDD_HHMMSS();
  const long NSAMPLES = nWaveformSamples_;
  // clang-format off
  if (
      // FPGA Type and Version
      fits_update_key_str(outputFile_, n = const_cast<char*>("FPGATYPE"), const_cast<char*>(fpgaTypeStr.c_str()),      "FPGA Type", &fitsStatus_) ||
      fits_update_key_str(outputFile_, n = const_cast<char*>("FPGAVERS"), const_cast<char*>(fpgaVersionStr.c_str()),   "FPGA Version", &fitsStatus_) ||
      fits_update_key_str(outputFile_, n = const_cast<char*>("FILEDATE"), const_cast<char*>(creationDate.c_str()),     "fileCreationDate", &fitsStatus_) ||
      fits_update_key_str(outputFile_, n = const_cast<char*>("DET_ID"),   const_cast<char*>(detectorID_.c_str()), "detectorID", &fitsStatus_) ||
      fits_update_key_lng(outputFile_, n = const_cast<char*>("NSAMPLES"), NSAMPLES, "nSamples", &fitsStatus_) ||
      fits_update_key_lng(outputFile_, n = const_cast<char*>("TIMERES"),  GROWTH_FY2015_ADC::TimeTagResolutionInNanoSec, "timeTagResolution in nano second", &fitsStatus_) ||
      fits_update_key_lng(outputFile_, n = const_cast<char*>("PHA_MIN"),  GROWTH_FY2015_ADC::PHAMinimum, "PHA range minimum", &fitsStatus_) ||
      fits_update_key_lng(outputFile_, n = const_cast<char*>("PHA_MAX"),  GROWTH_FY2015_ADC::PHAMaximum, "PHA range maximum", &fitsStatus_) ||
      fits_update_key_dbl(outputFile_, n = const_cast<char*>("EXPOSURE"), exposureInSec_, 2, "exposure specified via command line", &fitsStatus_)
      ) {  // clang-format on

    const std::string msg = fmt::format("Failed to update TTYPE comment for {}", n);
    spdlog::error(msg);
    throw std::runtime_error(msg);
  }

  // configurationYAML as HISTORY
  if (!configurationYAMLFile_.empty()) {
    std::vector<std::string> lines;
    std::ifstream ifs(configurationYAMLFile_);
    while (!ifs.eof()) {
      std::string line;
      std::getline(ifs, line);
      lines.push_back(line);
    }
    for (auto line : lines) {
      line = "YAML-- " + line;
      fits_write_history(outputFile_, const_cast<char*>(line.c_str()), &fitsStatus_);
    }
  }
}

void EventListFileFITS::reportErrorThenQuitIfError(int fitsStatus, const std::string& methodName) {
  if (fitsStatus) {  // if error
    const std::string msg = "FITS file manipulation error in " + methodName;
    spdlog::error(msg);
    fits_report_error(stderr, fitsStatus);
    throw std::runtime_error(methodName);
  }
}

void EventListFileFITS::fillGPSTime(
    const std::array<u8, GROWTH_FY2015_ADC::GPS_TIME_REG_SIZE_BYTES + 1>& gpsTimeRegisterBuffer) {
  // 0123456789X123456789
  // GPYYMMDDHHMMSSxxxxxx
  const i64 timeTag = [&]() {
    i64 timeTag = 0;
    for (size_t i = 14; i < GROWTH_FY2015_ADC::GPS_TIME_REG_SIZE_BYTES; i++) {
      timeTag = gpsTimeRegisterBuffer.at(i) + (timeTag << 8);
    }
    return timeTag;
  }();

  std::lock_guard<std::mutex> guard(fitsAccessMutex_);

  // move to GPS HDU
  fits_movnam_hdu(outputFile_, BINARY_TBL, const_cast<char*>("GPS"), 0, &fitsStatus_);
  reportErrorThenQuitIfError(fitsStatus_, __func__);

  rowIndexGPS_++;

  // get unix time
  const u32 unixTime = timeutil::getUNIXTimeAsUInt64();

  spdlog::debug("GPS timestamp = {}, UNIX timestamp = {}, FPGA timestamp = {}",
                std::string(reinterpret_cast<const char*>(gpsTimeRegisterBuffer.data()), 14), unixTime, timeTag);

  // write
  auto bufferPtr = const_cast<char*>(reinterpret_cast<const char*>(gpsTimeRegisterBuffer.data()));
  fitsStatus_ = 0;
  // clang-format off
  fits_write_col(outputFile_, TLONGLONG, COL_FPGA_TIMETAG, rowIndexGPS_, FIRST_ELEMENT, 1, const_cast<i64*>(&timeTag),   &fitsStatus_);
  fits_write_col(outputFile_, TUINT,     COL_UNIXTIME,     rowIndexGPS_, FIRST_ELEMENT, 1, const_cast<u32*>(&unixTime),  &fitsStatus_);
  fits_write_col(outputFile_, TSTRING,   COL_GPSTIME,      rowIndexGPS_, FIRST_ELEMENT, 1, &bufferPtr, &fitsStatus_);
  // clang-format on

  // move back to EVENTS HDU
  fits_movnam_hdu(outputFile_, BINARY_TBL, const_cast<char*>("EVENTS"), 0, &fitsStatus_);
  reportErrorThenQuitIfError(fitsStatus_, __func__);
}

void EventListFileFITS::fillEvents(const EventList& events) {
  std::lock_guard<std::mutex> guard(fitsAccessMutex_);
  for (auto& event : events) {
    rowIndexEvent_++;
    {  // clang-format off
      fits_write_col(outputFile_, TBYTE,     COL_CHANNEL,        rowIndexEvent_, FIRST_ELEMENT, 1, &event->ch,            &fitsStatus_);
      fits_write_col(outputFile_, TLONGLONG, COL_TIMETAG,        rowIndexEvent_, FIRST_ELEMENT, 1, &event->timeTag,       &fitsStatus_);
      fits_write_col(outputFile_, TUSHORT,   COL_TRIGGER_COUNT,  rowIndexEvent_, FIRST_ELEMENT, 1, &event->triggerCount,  &fitsStatus_);
      fits_write_col(outputFile_, TUSHORT,   COL_PHA_MAX,        rowIndexEvent_, FIRST_ELEMENT, 1, &event->phaMax,        &fitsStatus_);
      fits_write_col(outputFile_, TUSHORT,   COL_PHA_MAX_TIME,   rowIndexEvent_, FIRST_ELEMENT, 1, &event->phaMaxTime,    &fitsStatus_);
      fits_write_col(outputFile_, TUSHORT,   COL_PHA_MIN,        rowIndexEvent_, FIRST_ELEMENT, 1, &event->phaMin,        &fitsStatus_);
      fits_write_col(outputFile_, TUSHORT,   COL_PHA_FIRST,      rowIndexEvent_, FIRST_ELEMENT, 1, &event->phaFirst,      &fitsStatus_);
      fits_write_col(outputFile_, TUSHORT,   COL_PHA_LAST,       rowIndexEvent_, FIRST_ELEMENT, 1, &event->phaLast,       &fitsStatus_);
      fits_write_col(outputFile_, TUSHORT,   COL_MAX_DERIVATIVE, rowIndexEvent_, FIRST_ELEMENT, 1, &event->maxDerivative, &fitsStatus_);
      fits_write_col(outputFile_, TUSHORT,   COL_BASELINE,       rowIndexEvent_, FIRST_ELEMENT, 1, &event->baseline,      &fitsStatus_);
      if (nWaveformSamples_ != 0) {
        fits_write_col(outputFile_, TUSHORT, COL_WAVEFORM, rowIndexEvent_, FIRST_ELEMENT, nWaveformSamples_, event->waveform, &fitsStatus_);
      }
    }  // clang-format on
    expandIfNecessary();
  }
}

void EventListFileFITS::expandIfNecessary() {
  int fitsStatus = 0;
  // check heap size, and expand row size if necessary (to avoid slow down of cfitsio)
  while (rowIndexEvent_ > fitsNRowsEvent_) {
    const auto startTime = std::chrono::system_clock::now();
    fits_flush_file(outputFile_, &fitsStatus);
    fits_insert_rows(outputFile_, fitsNRowsEvent_ + 1, rowExpansionStep - 1, &fitsStatus);
    reportErrorThenQuitIfError(fitsStatus, __func__);

    long nRowsGot = 0;
    fits_get_num_rows(outputFile_, &nRowsGot, &fitsStatus);
    reportErrorThenQuitIfError(fitsStatus, __func__);
    fitsNRowsEvent_ = nRowsGot;

    rowExpansionStep = rowExpansionStep * 2;
    const auto endTime = std::chrono::system_clock::now();
    const auto elapsedTimeMicrosec = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
    spdlog::info("Output FITS file was resized to {} rows (took {:.2f} ms)", fitsNRowsEvent_,
                 elapsedTimeMicrosec / 1000.0);
  }
}

void EventListFileFITS::close() {
  std::lock_guard<std::mutex> guard(fitsAccessMutex_);
  if (outputFileIsOpen_) {
    outputFileIsOpen_ = false;
    int fitsStatus = 0;

    assert(rowIndexEvent_ <= fitsNRowsEvent_);
    const size_t nUnusedRow = fitsNRowsEvent_ - rowIndexEvent_;
    spdlog::info("Closing the current output file.");
    spdlog::info(" rowIndex    = {} (number of filled rows)", rowIndexEvent_);
    spdlog::info(" fitsNRows   = {} (allocated row number)", fitsNRowsEvent_);
    spdlog::info(" unused rows = {}", nUnusedRow);

    // Move to Event HDU
    fits_movnam_hdu(outputFile_, BINARY_TBL, const_cast<char*>("EVENTS"), 0, &fitsStatus_);
    reportErrorThenQuitIfError(fitsStatus_, __func__);

    // Delete unfilled rows.
    fits_flush_file(outputFile_, &fitsStatus);
    reportErrorThenQuitIfError(fitsStatus, __func__);
    if (fitsNRowsEvent_ != rowIndexEvent_) {
      spdlog::debug("Deleting unused {} rows.", nUnusedRow);
      fits_delete_rows(outputFile_, rowIndexEvent_ + 1, nUnusedRow, &fitsStatus);
      reportErrorThenQuitIfError(fitsStatus, __func__);
    }

    // Update NAXIS2
    fits_update_key(outputFile_, TULONG, const_cast<char*>("NAXIS2"), reinterpret_cast<void*>(&rowIndexEvent_),
                    const_cast<char*>("number of rows in table"), &fitsStatus);
    reportErrorThenQuitIfError(fitsStatus, __func__);
    if (rowIndexEvent_ == 0) {
      spdlog::debug("This HDU has 0 row.");
    }

    // Recover unused heap
    spdlog::debug("Recovering unused heap.");
    fits_compress_heap(outputFile_, &fitsStatus);
    reportErrorThenQuitIfError(fitsStatus, __func__);

    // Write date
    spdlog::debug("Writing date to file.");
    fits_write_date(outputFile_, &fitsStatus);
    reportErrorThenQuitIfError(fitsStatus, __func__);

    // Update checksum
    spdlog::debug("Updating checksum.");
    fits_write_chksum(outputFile_, &fitsStatus);
    reportErrorThenQuitIfError(fitsStatus, __func__);

    // Close FITS File
    spdlog::debug("Closing file.");
    fits_close_file(outputFile_, &fitsStatus);
    reportErrorThenQuitIfError(fitsStatus, __func__);
    spdlog::debug("Output FITS file closed.");
  }
}
