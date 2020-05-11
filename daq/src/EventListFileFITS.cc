#include "EventListFileFITS.hh"

EventListFileFITS::EventListFileFITS(std::string fileName, std::string detectorID, std::string configurationYAMLFile,
                                     size_t nSamples, double exposureInSec, uint32_t fpgaType, uint32_t fpgaVersion)
    : EventListFile(fileName),
      detectorID(std::move(detectorID)),
      nSamples(nSamples),
      configurationYAMLFile(std::move(configurationYAMLFile)),
      exposureInSec(exposureInSec),
      fpgaType(fpgaType),
      fpgaVersion(fpgaVersion) {
  createOutputFITSFile();
}

EventListFileFITS::~EventListFileFITS() { close(); }

void EventListFileFITS::createOutputFITSFile() {
  std::lock_guard<std::mutex> guard(fitsAccessMutex_);
  using namespace std;

  rowIndex = 0;
  rowIndex_GPS = 0;

  size_t nColumns = nColumns_Event;

  if (nSamples == 0) {
    nColumns--;  // delete waveform column
  } else {
    std::stringstream ss;
    ss << this->nSamples << "U";
    strcpy(tforms[nColumns - 1], ss.str().c_str());
  }

  long nRows = InitialRowNumber;
  fitsNRows = InitialRowNumber;
  long naxis = 1;
  long naxes[1] = {1};
  int tbltype = BINARY_TBL;
  // Create FITS File
  if (fileName_[0] != '!') {
    fileName_ = "!" + fileName_;
  }
  fits_create_file(&outputFile, fileName_.c_str(), &fitsStatus);
  this->reportErrorThenQuitIfError(fitsStatus, __func__);

  // Create FITS Image
  fits_create_img(outputFile, USHORT_IMG, naxis, naxes, &fitsStatus);
  this->reportErrorThenQuitIfError(fitsStatus, __func__);

  // Create BINTABLE
  fits_create_tbl(outputFile, tbltype, nRows, nColumns, const_cast<char**>(ttypes), tforms, const_cast<char**>(tunits),
                  "EVENTS", &fitsStatus);
  this->reportErrorThenQuitIfError(fitsStatus, __func__);

  // write header info
  writeHeader();

  // Create GPS Time HDU
  fits_create_tbl(outputFile, tbltype, InitialRowNumber_GPS, nColumns_GPS, const_cast<char**>(ttypes_GPS),
                  const_cast<char**>(tforms_GPS), const_cast<char**>(tunits_GPS), "GPS", &fitsStatus);
  this->reportErrorThenQuitIfError(fitsStatus, __func__);

  fits_movnam_hdu(outputFile, tbltype, const_cast<char*>("EVENTS"), 0, &fitsStatus);
  this->reportErrorThenQuitIfError(fitsStatus, __func__);

  outputFileIsOpen = true;
}

void EventListFileFITS::writeHeader() {
  /* sample code
   fits_update_key_str_check(fp, n=(char*)"TELESCOP", v->telescop, c->telescop, &istat)||
   fits_update_key_fixdbl(fp, n=(char*)"dec_pnt", v->dec_pnt, 8, c->dec_pnt, &istat)||
   fits_update_key_lng(fp, n=(char*)"EQUINOX", v->equinox, c->equinox, &istat)||
   *
   */

  char* n;  // keyword name
  std::string fpgaTypeStr = CxxUtilities::String::toHexString(fpgaType, 8);
  std::string fpgaVersionStr = CxxUtilities::String::toHexString(fpgaVersion, 8);
  std::string creationDate = CxxUtilities::Time::getCurrentTimeYYYYMMDD_HHMMSS();
  long NSAMPLES = this->nSamples;
  // clang-format off
  if (
      // FPGA Type and Version
      fits_update_key_str(outputFile, n = const_cast<char*>("FPGATYPE"), const_cast<char*>(fpgaTypeStr.c_str()),      "FPGA Type", &fitsStatus) ||
      fits_update_key_str(outputFile, n = const_cast<char*>("FPGAVERS"), const_cast<char*>(fpgaVersionStr.c_str()),   "FPGA Version", &fitsStatus) ||
      fits_update_key_str(outputFile, n = const_cast<char*>("FILEDATE"), const_cast<char*>(creationDate.c_str()),     "fileCreationDate", &fitsStatus) ||
      fits_update_key_str(outputFile, n = const_cast<char*>("DET_ID"),   const_cast<char*>(this->detectorID.c_str()), "detectorID", &fitsStatus) ||
      fits_update_key_lng(outputFile, n = const_cast<char*>("NSAMPLES"), NSAMPLES, "nSamples", &fitsStatus) ||
      fits_update_key_lng(outputFile, n = const_cast<char*>("TIMERES"),  GROWTH_FY2015_ADC::TimeTagResolutionInNanoSec, "timeTagResolution in nano second", &fitsStatus) ||
      fits_update_key_lng(outputFile, n = const_cast<char*>("PHA_MIN"),  GROWTH_FY2015_ADC::PHAMinimum, "PHA range minimum", &fitsStatus) ||
      fits_update_key_lng(outputFile, n = const_cast<char*>("PHA_MAX"),  GROWTH_FY2015_ADC::PHAMaximum, "PHA range maximum", &fitsStatus) ||
      fits_update_key_dbl(outputFile, n = const_cast<char*>("EXPOSURE"), this->exposureInSec, 2, "exposure specified via command line", &fitsStatus)
      ) {  // clang-format on
    using namespace std;
    cerr << "Error: while updating TTYPE comment for " << n << endl;
    exit(-1);
  }

  // configurationYAML as HISTORY
  if (configurationYAMLFile != "") {
    std::vector<std::string> lines = CxxUtilities::File::getAllLines(configurationYAMLFile);
    for (auto line : lines) {
      line = "YAML-- " + line;
      fits_write_history(outputFile, const_cast<char*>(line.c_str()), &fitsStatus);
    }
  }
}

void reportErrorThenQuitIfError(int fitsStatus, std::string methodName) {
  if (fitsStatus) {  // if error
    using namespace std;
    cerr << "Error (" << methodName << "):";
    fits_report_error(stderr, fitsStatus);
    exit(-1);
  }
}

void EventListFileFITS::fillGPSTime(const uint8_t* gpsTimeRegisterBuffer) {
  // 0123456789X123456789
  // GPYYMMDDHHMMSSxxxxxx
  long long timeTag = 0;
  for (size_t i = 14; i < GROWTH_FY2015_ADC::LengthOfGPSTimeRegister; i++) {
    timeTag = gpsTimeRegisterBuffer[i] + (timeTag << 8);
  }
  std::lock_guard<std::mutex> guard(fitsAccessMutex_);
  // move to GPS HDU
  fits_movnam_hdu(outputFile, BINARY_TBL, const_cast<char*>("GPS"), 0, &fitsStatus);
  this->reportErrorThenQuitIfError(fitsStatus, __func__);

  rowIndex_GPS++;

  // get unix time
  long unixTime = CxxUtilities::Time::getUNIXTimeAsUInt32();

  // write
  fits_write_col(outputFile, TLONGLONG, Column_fpgaTimeTag, rowIndex_GPS, firstElement, 1, &timeTag, &fitsStatus);
  fits_write_col(outputFile, TLONG, Column_unixTime, rowIndex_GPS, firstElement, 1, &unixTime, &fitsStatus);
  fits_write_col(outputFile, TSTRING, Column_gpsTime, rowIndex_GPS, firstElement, 1 /* YYMMDDHHMMSS */,
                 &gpsTimeRegisterBuffer, &fitsStatus);

  // move back to EVENTS HDU
  fits_movnam_hdu(outputFile, BINARY_TBL, const_cast<char*>("EVENTS"), 0, &fitsStatus);
  this->reportErrorThenQuitIfError(fitsStatus, __func__);
}

void EventListFileFITS::fillEvents(const std::vector<GROWTH_FY2015_ADC_Type::Event*>& events) {
  std::lock_guard<std::mutex> guard(fitsAccessMutex_);
  for (auto& event : events) {
    rowIndex++;
    {  // clang-format off
      fits_write_col(outputFile, TBYTE, Column_boardIndexAndChannel, rowIndex, firstElement, 1, &event->ch, &fitsStatus);
      fits_write_col(outputFile, TLONGLONG, Column_timeTag, rowIndex, firstElement, 1, &event->timeTag, &fitsStatus);
      fits_write_col(outputFile, TUSHORT, Column_triggerCount, rowIndex, firstElement, 1, &event->triggerCount, &fitsStatus);
      fits_write_col(outputFile, TUSHORT, Column_phaMax, rowIndex, firstElement, 1, &event->phaMax, &fitsStatus);
      fits_write_col(outputFile, TUSHORT, Column_phaMaxTime, rowIndex, firstElement, 1, &event->phaMaxTime, &fitsStatus);
      fits_write_col(outputFile, TUSHORT, Column_phaMin, rowIndex, firstElement, 1, &event->phaMin, &fitsStatus);
      fits_write_col(outputFile, TUSHORT, Column_phaFirst, rowIndex, firstElement, 1, &event->phaFirst, &fitsStatus);
      fits_write_col(outputFile, TUSHORT, Column_phaLast, rowIndex, firstElement, 1, &event->phaLast, &fitsStatus);
      fits_write_col(outputFile, TUSHORT, Column_maxDerivative, rowIndex, firstElement, 1, &event->maxDerivative, &fitsStatus);
      fits_write_col(outputFile, TUSHORT, Column_baseline, rowIndex, firstElement, 1, &event->baseline, &fitsStatus);
      if (nSamples != 0) {
        fits_write_col(outputFile, TUSHORT, Column_waveform, rowIndex, firstElement, nSamples, event->waveform, &fitsStatus);
      }
    }  // clang-format on
    expandIfNecessary();
  }
}

void EventListFileFITS::expandIfNecessary() {
  using namespace std;
  int fitsStatus = 0;
  // check heap size, and expand row size if necessary (to avoid slow down of cfitsio)
  while (rowIndex > fitsNRows) {
    fits_flush_file(outputFile, &fitsStatus);
    fits_insert_rows(outputFile, fitsNRows + 1, rowExpansionStep - 1, &fitsStatus);
    this->reportErrorThenQuitIfError(fitsStatus, __func__);

    long nRowsGot = 0;
    fits_get_num_rows(outputFile, &nRowsGot, &fitsStatus);
    this->reportErrorThenQuitIfError(fitsStatus, __func__);
    fitsNRows = nRowsGot;

    rowExpansionStep = rowExpansionStep * 2;
    cout << "Output FITS file was resized to " << dec << fitsNRows << " rows." << endl;
  }
}

void EventListFileFITS::close() {
  if (outputFileIsOpen) {
    std::lock_guard<std::mutex> guard(fitsAccessMutex_);
    outputFileIsOpen = false;
    using namespace std;
    int fitsStatus = 0;

    uint32_t nUnusedRow = fitsNRows - rowIndex;
    cout << "Closing the current output file." << endl;
    cout << " rowIndex    = " << dec << rowIndex << " (number of filled rows)" << endl;
    cout << " fitsNRows   = " << dec << fitsNRows << " (allocated row number)" << endl;
    cout << " unused rows = " << dec << nUnusedRow << endl;

    /* Delete unfilled rows. */
    fits_flush_file(outputFile, &fitsStatus);
    this->reportErrorThenQuitIfError(fitsStatus, __func__);
    if (fitsNRows != rowIndex) {
      cout << "Deleting unused " << nUnusedRow << " rows." << endl;
      fits_delete_rows(outputFile, rowIndex + 1, nUnusedRow, &fitsStatus);
      this->reportErrorThenQuitIfError(fitsStatus, __func__);
    }

    /* Recover unused heap. */
    cout << "Recovering unused heap." << endl;
    fits_compress_heap(outputFile, &fitsStatus);
    this->reportErrorThenQuitIfError(fitsStatus, __func__);

    /* Write date. */
    cout << "Writing data to file." << endl;
    fits_write_date(outputFile, &fitsStatus);
    this->reportErrorThenQuitIfError(fitsStatus, __func__);

    /* Update NAXIS2 */
    if (rowIndex == 0) {
      fits_update_key(outputFile, TULONG, const_cast<char*>("NAXIS2"), reinterpret_cast<void*>(&rowIndex),
                      const_cast<char*>("number of rows in table"), &fitsStatus);
      this->reportErrorThenQuitIfError(fitsStatus, __func__);
      cout << "This HDU has 0 row." << endl;
    }

    /* Update checksum. */
    cout << "Updating ch ecksum." << endl;
    fits_write_chksum(outputFile, &fitsStatus);
    this->reportErrorThenQuitIfError(fitsStatus, __func__);

    /* Close FITS File */
    cout << "Closing file." << endl;
    fits_close_file(outputFile, &fitsStatus);
    this->reportErrorThenQuitIfError(fitsStatus, __func__);
    cout << "Output FITS file closed." << endl;
  }
}