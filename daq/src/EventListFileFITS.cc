#include "EventListFileFITS.hh"

#include "timeutil.hh"
#include "stringutil.hh"
EventListFileFITS::EventListFileFITS(const std::string& fileName, const std::string& detectorID,
                                     const std::string& configurationYAMLFile, size_t nSamples, f64 exposureInSec,
                                     u32 fpgaType, u32 fpgaVersion)
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


  rowIndex = 0;
  rowIndexGPS = 0;

  size_t nColumns = NumColumnsEvent;

  if (nSamples == 0) {
    nColumns--;  // delete waveform column
  } else {
    std::stringstream ss;
    ss << nSamples << "U";
    strcpy(tforms[nColumns - 1], ss.str().c_str());
  }

  fitsNRows = InitialRowNumberEvent;
  long naxis = 1;
  long naxes[1] = {1};
  i32 tbltype = BINARY_TBL;
  // Create FITS File
  if (fileName_[0] != '!') {
    fileName_ = "!" + fileName_;
  }
  fits_create_file(&outputFile, fileName_.c_str(), &fitsStatus);
  reportErrorThenQuitIfError(fitsStatus, __func__);

  // Create FITS Image
  fits_create_img(outputFile, USHORT_IMG, naxis, naxes, &fitsStatus);
  reportErrorThenQuitIfError(fitsStatus, __func__);

  // Create BINTABLE
  fits_create_tbl(outputFile, tbltype, fitsNRows, nColumns, const_cast<char**>(ttypes), tforms,
                  const_cast<char**>(tunits), "EVENTS", &fitsStatus);
  reportErrorThenQuitIfError(fitsStatus, __func__);

  // write header info
  writeHeader();

  // Create GPS Time HDU
  fits_create_tbl(outputFile, tbltype, InitialRowNumberGPS, NumColumnsGPS, const_cast<char**>(ttypesGPS),
                  const_cast<char**>(tformsGPS), const_cast<char**>(tunitsGPS), "GPS", &fitsStatus);
  reportErrorThenQuitIfError(fitsStatus, __func__);

  fits_movnam_hdu(outputFile, tbltype, const_cast<char*>("EVENTS"), 0, &fitsStatus);
  reportErrorThenQuitIfError(fitsStatus, __func__);

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
  std::string fpgaTypeStr = stringutil::toHexString(fpgaType, 8);
  std::string fpgaVersionStr = stringutil::toHexString(fpgaVersion, 8);
  std::string creationDate = timeutil::getCurrentTimeYYYYMMDD_HHMMSS();
  long NSAMPLES = nSamples;
  // clang-format off
  if (
      // FPGA Type and Version
      fits_update_key_str(outputFile, n = const_cast<char*>("FPGATYPE"), const_cast<char*>(fpgaTypeStr.c_str()),      "FPGA Type", &fitsStatus) ||
      fits_update_key_str(outputFile, n = const_cast<char*>("FPGAVERS"), const_cast<char*>(fpgaVersionStr.c_str()),   "FPGA Version", &fitsStatus) ||
      fits_update_key_str(outputFile, n = const_cast<char*>("FILEDATE"), const_cast<char*>(creationDate.c_str()),     "fileCreationDate", &fitsStatus) ||
      fits_update_key_str(outputFile, n = const_cast<char*>("DET_ID"),   const_cast<char*>(detectorID.c_str()), "detectorID", &fitsStatus) ||
      fits_update_key_lng(outputFile, n = const_cast<char*>("NSAMPLES"), NSAMPLES, "nSamples", &fitsStatus) ||
      fits_update_key_lng(outputFile, n = const_cast<char*>("TIMERES"),  GROWTH_FY2015_ADC::TimeTagResolutionInNanoSec, "timeTagResolution in nano second", &fitsStatus) ||
      fits_update_key_lng(outputFile, n = const_cast<char*>("PHA_MIN"),  GROWTH_FY2015_ADC::PHAMinimum, "PHA range minimum", &fitsStatus) ||
      fits_update_key_lng(outputFile, n = const_cast<char*>("PHA_MAX"),  GROWTH_FY2015_ADC::PHAMaximum, "PHA range maximum", &fitsStatus) ||
      fits_update_key_dbl(outputFile, n = const_cast<char*>("EXPOSURE"), exposureInSec, 2, "exposure specified via command line", &fitsStatus)
      ) {  // clang-format on

    cerr << "Error: while updating TTYPE comment for " << n << std::endl;
    exit(-1);
  }

  // configurationYAML as HISTORY
  if (configurationYAMLFile != "") {
    std::vector<std::string> lines;
    std::ifstream ifs(configurationYAMLFile);
    while(!ifs.eof()){
    	std::string line;
    	std::getline(ifs, line);
    	lines.push_back(line);
    }
    for (auto line : lines) {
      line = "YAML-- " + line;
      fits_write_history(outputFile, const_cast<char*>(line.c_str()), &fitsStatus);
    }
  }
}

void EventListFileFITS::reportErrorThenQuitIfError(int fitsStatus, const std::string& methodName) {
  if (fitsStatus) {  // if error
    std::cerr << "Error (" << methodName << "):";
    fits_report_error(stderr, fitsStatus);
    exit(-1);
  }
}

void EventListFileFITS::fillGPSTime(const u8* gpsTimeRegisterBuffer) {
  // 0123456789X123456789
  // GPYYMMDDHHMMSSxxxxxx
  i64 timeTag = 0;
  for (size_t i = 14; i < GROWTH_FY2015_ADC::LengthOfGPSTimeRegister; i++) {
    timeTag = gpsTimeRegisterBuffer[i] + (timeTag << 8);
  }
  std::lock_guard<std::mutex> guard(fitsAccessMutex_);
  // move to GPS HDU
  fits_movnam_hdu(outputFile, BINARY_TBL, const_cast<char*>("GPS"), 0, &fitsStatus);
  reportErrorThenQuitIfError(fitsStatus, __func__);

  rowIndexGPS++;

  // get unix time
  const u32 unixTime = timeutil::getUNIXTimeAsUInt64();

  // write
  fits_write_col(outputFile, TLONGLONG, Column_fpgaTimeTag, rowIndexGPS, firstElement, 1, &timeTag, &fitsStatus);
  fits_write_col(outputFile, TLONG, Column_unixTime, rowIndexGPS, firstElement, 1,
		  const_cast<u32*>(&unixTime), &fitsStatus);
  fits_write_col(outputFile, TSTRING, Column_gpsTime, rowIndexGPS, firstElement, 1 /* YYMMDDHHMMSS */,
                 &gpsTimeRegisterBuffer, &fitsStatus);

  // move back to EVENTS HDU
  fits_movnam_hdu(outputFile, BINARY_TBL, const_cast<char*>("EVENTS"), 0, &fitsStatus);
  reportErrorThenQuitIfError(fitsStatus, __func__);
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
  int fitsStatus = 0;
  // check heap size, and expand row size if necessary (to avoid slow down of cfitsio)
  while (rowIndex > fitsNRows) {
    fits_flush_file(outputFile, &fitsStatus);
    fits_insert_rows(outputFile, fitsNRows + 1, rowExpansionStep - 1, &fitsStatus);
    reportErrorThenQuitIfError(fitsStatus, __func__);

    long nRowsGot = 0;
    fits_get_num_rows(outputFile, &nRowsGot, &fitsStatus);
    reportErrorThenQuitIfError(fitsStatus, __func__);
    fitsNRows = nRowsGot;

    rowExpansionStep = rowExpansionStep * 2;
    std::cout << "Output FITS file was resized to " << std::dec << fitsNRows << " rows." << std::endl;
  }
}

void EventListFileFITS::close() {
  if (outputFileIsOpen) {
    std::lock_guard<std::mutex> guard(fitsAccessMutex_);
    outputFileIsOpen = false;
    int fitsStatus = 0;

    u32 nUnusedRow = fitsNRows - rowIndex;
    std::cout << "Closing the current output file." << std::endl;
    std::cout << " rowIndex    = " << std::dec << rowIndex << " (number of filled rows)" << std::endl;
    std::cout << " fitsNRows   = " << std::dec << fitsNRows << " (allocated row number)" << std::endl;
    std::cout << " unused rows = " << std::dec << nUnusedRow << std::endl;

    /* Delete unfilled rows. */
    fits_flush_file(outputFile, &fitsStatus);
    reportErrorThenQuitIfError(fitsStatus, __func__);
    if (fitsNRows != rowIndex) {
      std::cout << "Deleting unused " << nUnusedRow << " rows." << std::endl;
      fits_delete_rows(outputFile, rowIndex + 1, nUnusedRow, &fitsStatus);
      reportErrorThenQuitIfError(fitsStatus, __func__);
    }

    /* Recover unused heap. */
    std::cout << "Recovering unused heap." << std::endl;
    fits_compress_heap(outputFile, &fitsStatus);
    reportErrorThenQuitIfError(fitsStatus, __func__);

    /* Write date. */
    std::cout << "Writing data to file." << std::endl;
    fits_write_date(outputFile, &fitsStatus);
    reportErrorThenQuitIfError(fitsStatus, __func__);

    /* Update NAXIS2 */
    if (rowIndex == 0) {
      fits_update_key(outputFile, TULONG, const_cast<char*>("NAXIS2"), reinterpret_cast<void*>(&rowIndex),
                      const_cast<char*>("number of rows in table"), &fitsStatus);
      reportErrorThenQuitIfError(fitsStatus, __func__);
      std::cout << "This HDU has 0 row." << std::endl;
    }

    /* Update checksum. */
    std::cout << "Updating checksum." << std::endl;
    fits_write_chksum(outputFile, &fitsStatus);
    reportErrorThenQuitIfError(fitsStatus, __func__);

    /* Close FITS File */
    std::cout << "Closing file." << std::endl;
    fits_close_file(outputFile, &fitsStatus);
    reportErrorThenQuitIfError(fitsStatus, __func__);
    std::cout << "Output FITS file closed." << std::endl;
  }
}
