#include "EventListFileFITS.hh"


  EventListFileFITS::EventListFileFITS(std::string fileName, std::string detectorID = "empty", std::string configurationYAMLFile = "",
                    size_t nSamples = 1024, double exposureInSec = 0,  //
                    uint32_t fpgaType = 0x00000000, uint32_t fpgaVersion = 0x00000000)
      : EventListFile(fileName),
        detectorID(detectorID),
        nSamples(nSamples),  //
        configurationYAMLFile(configurationYAMLFile),
        exposureInSec(exposureInSec),  //
        fpgaType(fpgaType),
        fpgaVersion(fpgaVersion) {
    createOutputFITSFile();
  }
  EventListFileFITS::~EventListFileFITS() { close(); }

  void EventListFileFITS::createOutputFITSFile() {
    fitsAccessMutes.lock();
    using namespace std;

    rowIndex     = 0;
    rowIndex_GPS = 0;

    size_t nColumns = nColumns_Event;

    if (nSamples == 0) {
      nColumns--;  // delete waveform column
    } else {
      std::stringstream ss;
      ss << this->nSamples << "U";
      strcpy(tforms[nColumns - 1], ss.str().c_str());
    }

    long nRows    = InitialRowNumber;
    fitsNRows     = InitialRowNumber;
    long naxis    = 1;
    long naxes[1] = {1};
    int tbltype   = BINARY_TBL;
    // Create FITS File
    if (fileName[0] != '!') { fileName = "!" + fileName; }
    fits_create_file(&outputFile, fileName.c_str(), &fitsStatus);
    this->reportErrorThenQuitIfError(fitsStatus, __func__);

    // Create FITS Image
    fits_create_img(outputFile, USHORT_IMG, naxis, naxes, &fitsStatus);
    this->reportErrorThenQuitIfError(fitsStatus, __func__);

    // Create BINTABLE
    fits_create_tbl(outputFile, tbltype, nRows, nColumns, ttypes, tforms, tunits, "EVENTS", &fitsStatus);
    this->reportErrorThenQuitIfError(fitsStatus, __func__);

    // write header info
    writeHeader();

    // Create GPS Time HDU
    fits_create_tbl(outputFile, tbltype, InitialRowNumber_GPS, nColumns_GPS, ttypes_GPS, tforms_GPS, tunits_GPS, "GPS",
                    &fitsStatus);
    this->reportErrorThenQuitIfError(fitsStatus, __func__);

    fits_movnam_hdu(outputFile, tbltype, (char*)"EVENTS", 0, &fitsStatus);
    this->reportErrorThenQuitIfError(fitsStatus, __func__);

    outputFileIsOpen = true;
    fitsAccessMutes.unlock();
  }



  void EventListFileFITS::writeHeader() {
    /* sample code
     fits_update_key_str_check(fp, n=(char*)"TELESCOP", v->telescop, c->telescop, &istat)||
     fits_update_key_fixdbl(fp, n=(char*)"dec_pnt", v->dec_pnt, 8, c->dec_pnt, &istat)||
     fits_update_key_lng(fp, n=(char*)"EQUINOX", v->equinox, c->equinox, &istat)||
     *
     */

    char* n;  // keyword name
    std::string fpgaTypeStr    = CxxUtilities::String::toHexString(fpgaType, 8);
    std::string fpgaVersionStr = CxxUtilities::String::toHexString(fpgaVersion, 8);
    std::string creationDate   = CxxUtilities::Time::getCurrentTimeYYYYMMDD_HHMMSS();
    long NSAMPLES              = this->nSamples;
    if (  //
          // FPGA Type and Version
        fits_update_key_str(outputFile, n = (char*)"FPGATYPE", (char*)fpgaTypeStr.c_str(), "FPGA Type",
                            &fitsStatus) ||  //
        fits_update_key_str(outputFile, n = (char*)"FPGAVERS", (char*)fpgaVersionStr.c_str(), "FPGA Version",
                            &fitsStatus) ||  //
                                             // fileCreationDate
        fits_update_key_str(outputFile, n = (char*)"FILEDATE", (char*)creationDate.c_str(), "fileCreationDate",
                            &fitsStatus) ||  //
        // detectorID
        fits_update_key_str(outputFile, n = (char*)"DET_ID", (char*)this->detectorID.c_str(), "detectorID",
                            &fitsStatus) ||  //
        // nSamples
        fits_update_key_lng(outputFile, n = (char*)"NSAMPLES", NSAMPLES, "nSamples", &fitsStatus) ||  //
        // timeTagResolution
        fits_update_key_lng(outputFile, n = (char*)"TIMERES", GROWTH_FY2015_ADC::TimeTagResolutionInNanoSec,
                            "timeTagResolution in nano second", &fitsStatus) ||  //
        // PHA min/max
        fits_update_key_lng(outputFile, n = (char*)"PHA_MIN", GROWTH_FY2015_ADC::PHAMinimum, "PHA range minimum",
                            &fitsStatus) ||  //
        fits_update_key_lng(outputFile, n = (char*)"PHA_MAX", GROWTH_FY2015_ADC::PHAMaximum, "PHA range maximum",
                            &fitsStatus) ||  //
        // exposure
        fits_update_key_dbl(outputFile, n = (char*)"EXPOSURE", this->exposureInSec, 2,
                            "exposure specified via command line", &fitsStatus)  //

    ) {
      using namespace std;
      cerr << "Error: while updating TTYPE comment for " << n << endl;
      exit(-1);
    }

    // configurationYAML as HISTORY
    if (configurationYAMLFile != "") {
      std::vector<std::string> lines = CxxUtilities::File::getAllLines(configurationYAMLFile);
      for (auto line : lines) {
        line = "YAML-- " + line;
        fits_write_history(outputFile, (char*)line.c_str(), &fitsStatus);
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

  void EventListFileFITS::fillGPSTime(uint8_t* gpsTimeRegisterBuffer) {
    // 0123456789X123456789
    // GPYYMMDDHHMMSSxxxxxx
    long long timeTag = 0;
    for (size_t i = 14; i < GROWTH_FY2015_ADC::LengthOfGPSTimeRegister; i++) {
      timeTag = gpsTimeRegisterBuffer[i] + (timeTag << 8);
    }
    fitsAccessMutes.lock();
    // move to GPS HDU
    fits_movnam_hdu(outputFile, BINARY_TBL, (char*)"GPS", 0, &fitsStatus);
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
    fits_movnam_hdu(outputFile, BINARY_TBL, (char*)"EVENTS", 0, &fitsStatus);
    this->reportErrorThenQuitIfError(fitsStatus, __func__);
    fitsAccessMutes.unlock();
  }

  void EventListFileFITS::fillEvents(std::vector<GROWTH_FY2015_ADC_Type::Event*>& events) {
    fitsAccessMutes.lock();
    for (auto& event : events) {
      rowIndex++;
      // ch
      fits_write_col(outputFile, TBYTE, Column_boardIndexAndChannel, rowIndex, firstElement, 1, &event->ch,
                     &fitsStatus);
      // timeTag
      fits_write_col(outputFile, TLONGLONG, Column_timeTag, rowIndex, firstElement, 1, &event->timeTag, &fitsStatus);
      // triggerCount
      fits_write_col(outputFile, TUSHORT, Column_triggerCount, rowIndex, firstElement, 1, &event->triggerCount,
                     &fitsStatus);
      // phaMax
      fits_write_col(outputFile, TUSHORT, Column_phaMax, rowIndex, firstElement, 1, &event->phaMax, &fitsStatus);
      // phaMaxTime
      fits_write_col(outputFile, TUSHORT, Column_phaMaxTime, rowIndex, firstElement, 1, &event->phaMaxTime,
                     &fitsStatus);
      // phaMin
      fits_write_col(outputFile, TUSHORT, Column_phaMin, rowIndex, firstElement, 1, &event->phaMin, &fitsStatus);
      // phaFirst
      fits_write_col(outputFile, TUSHORT, Column_phaFirst, rowIndex, firstElement, 1, &event->phaFirst, &fitsStatus);
      // phaLast
      fits_write_col(outputFile, TUSHORT, Column_phaLast, rowIndex, firstElement, 1, &event->phaLast, &fitsStatus);
      // maxDerivative
      fits_write_col(outputFile, TUSHORT, Column_maxDerivative, rowIndex, firstElement, 1, &event->maxDerivative,
                     &fitsStatus);
      // baseline
      fits_write_col(outputFile, TUSHORT, Column_baseline, rowIndex, firstElement, 1, &event->baseline, &fitsStatus);
      if (nSamples != 0) {
        // waveform
        fits_write_col(outputFile, TUSHORT, Column_waveform, rowIndex, firstElement, nSamples, event->waveform,
                       &fitsStatus);
      }

      //
      expandIfNecessary();
    }
    fitsAccessMutes.unlock();
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
      fitsAccessMutes.lock();
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
        fits_update_key(outputFile, TULONG, (char*)"NAXIS2", (void*)&rowIndex, (char*)"number of rows in table",
                        &fitsStatus);
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

      fitsAccessMutes.unlock();
    }
  }
