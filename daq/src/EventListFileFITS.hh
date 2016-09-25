/*
 * EventListFileFITS.hh
 *
 *  Created on: Sep 30, 2015
 *      Author: yuasa
 */
#ifndef EVENTLISTFILEFITS_HH_
#define EVENTLISTFILEFITS_HH_

extern "C" {
#include "fitsio.h"
}

#include "CxxUtilities/FitsUtility.hh"
#include "GROWTH_FY2015_ADC.hh"
#include "EventListFile.hh"

class EventListFileFITS: public EventListFile {
private:
	fitsfile* outputFile;
	std::string detectorID;
	GROWTH_FY2015_ADC_Type::Event eventEntry;
	std::string configurationYAMLFile;

private:
	//---------------------------------------------
	// event list HDU
	//---------------------------------------------
	static const size_t nColumns_Event = 11;
	char* ttypes[nColumns_Event] = { //
			(char*) "boardIndexAndChannel", //B
					(char*) "timeTag", //K
					(char*) "triggerCount", //U
					(char*) "phaMax", //U
					(char*) "phaMaxTime", //U
					(char*) "phaMin", //U
					(char*) "phaFirst", //U
					(char*) "phaLast", //U
					(char*) "maxDerivative", //U
					(char*) "baseline", //U
					(char*) "waveform" //B
			};
	const size_t MaxTFORM = 1024;
	char* tforms[nColumns_Event] = { //
			(char*) "B"/*uint8_t*/, //boardIndexAndChannel
					(char*) "K"/*uint64_t*/, //timeTag
					(char*) "U"/*uint16_t*/, //triggerCount
					(char*) "U"/*uint16_t*/, //phaMax
					(char*) "U"/*uint16_t*/, //phaMaxTime
					(char*) "U"/*uint16_t*/, //phaMin
					(char*) "U"/*uint16_t*/, //phaFirst
					(char*) "U"/*uint16_t*/, //phaLast
					(char*) "U"/*uint16_t*/, //maxDerivative
					(char*) "U"/*uint16_t*/, //baseline
					new char[MaxTFORM] /* to be filled later */ //
			};
	char* tunits[nColumns_Event] = { //
			(char*) "", (char*) "", (char*) "", (char*) "", (char*) "", (char*) "" //
			};
	enum columnIndices {
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
	static const size_t nColumns_GPS = 3;
	char* ttypes_GPS[nColumns_GPS] = { //
			(char*) "fpgaTimeTag", //
					(char*) "unixTime", //
					(char*) "gpsTime" //
			};
	char* tforms_GPS[nColumns_GPS] = { //
			(char*) "K", //
					(char*) "V", //
					(char*) "14A" //
			};
	char* tunits_GPS[nColumns_GPS] = { //
			(char*) "", //
					(char*) "", //
					(char*) "" //
			};
	enum columnIndices_GPS {
		Column_fpgaTimeTag = 1, //
		Column_unixTime = 2, //
		Column_gpsTime = 3
	};

private:
	static const size_t InitialRowNumber = 1000;
	static const size_t InitialRowNumber_GPS = 1000;
	int fitsStatus = 0;
	bool outputFileIsOpen = false;
	size_t rowIndex; // will be initialized in createOutputFITSFile()
	size_t rowIndex_GPS; // will be initialized in createOutputFITSFile()
	size_t fitsNRows; //currently allocated rows
	size_t rowExpansionStep = InitialRowNumber;
	size_t nSamples;
	double exposureInSec;
	uint32_t fpgaType = 0x00000000;
	uint32_t fpgaVersion = 0x00000000;

private:
	CxxUtilities::Mutex fitsAccessMutes;

public:
	EventListFileFITS(std::string fileName, std::string detectorID = "empty", std::string configurationYAMLFile = "",
			size_t nSamples = 1024, double exposureInSec = 0, //
			uint32_t fpgaType = 0x00000000, uint32_t fpgaVersion = 0x00000000) :
    EventListFile(fileName), detectorID(detectorID), nSamples(nSamples),//
    configurationYAMLFile(configurationYAMLFile), exposureInSec(exposureInSec),//
    fpgaType(fpgaType), fpgaVersion(fpgaVersion) {
		createOutputFITSFile();
	}

private:
	void createOutputFITSFile() {
		fitsAccessMutes.lock();
		using namespace std;

		rowIndex = 0;
		rowIndex_GPS = 0;

		size_t nColumns = nColumns_Event;

		if (nSamples == 0) {
			nColumns--; //delete waveform column
		} else {
			std::stringstream ss;
			ss << this->nSamples << "U";
			strcpy(tforms[nColumns - 1], ss.str().c_str());
		}

		long nRows = InitialRowNumber;
		fitsNRows = InitialRowNumber;
		long naxis = 1;
		long naxes[1] = { 1 };
		int tbltype = BINARY_TBL;
		// Create FITS File
		if (fileName[0] != '!') {
			fileName = "!" + fileName;
		}
		fits_create_file(&outputFile, fileName.c_str(), &fitsStatus);
		this->reportErrorThenQuitIfError(fitsStatus, __func__);

		// Create FITS Image
		fits_create_img(outputFile, USHORT_IMG, naxis, naxes, &fitsStatus);
		this->reportErrorThenQuitIfError(fitsStatus, __func__);

		// Create BINTABLE
		fits_create_tbl(outputFile, tbltype, nRows, nColumns, ttypes, tforms, tunits, "EVENTS", &fitsStatus);
		this->reportErrorThenQuitIfError(fitsStatus, __func__);

		//write header info
		writeHeader();

		// Create GPS Time HDU
		fits_create_tbl(outputFile, tbltype, InitialRowNumber_GPS, nColumns_GPS, ttypes_GPS, tforms_GPS, tunits_GPS, "GPS",
				&fitsStatus);
		this->reportErrorThenQuitIfError(fitsStatus, __func__);

		fits_movnam_hdu(outputFile, tbltype, (char*) "EVENTS", 0, &fitsStatus);
		this->reportErrorThenQuitIfError(fitsStatus, __func__);

		outputFileIsOpen = true;
		fitsAccessMutes.unlock();
	}

public:
	~EventListFileFITS() {
		close();
	}

private:
	void writeHeader() {
		/* sample code
		 fits_update_key_str_check(fp, n=(char*)"TELESCOP", v->telescop, c->telescop, &istat)||
		 fits_update_key_fixdbl(fp, n=(char*)"dec_pnt", v->dec_pnt, 8, c->dec_pnt, &istat)||
		 fits_update_key_lng(fp, n=(char*)"EQUINOX", v->equinox, c->equinox, &istat)||
		 *
		 */

		char* n; //keyword name
		std::string fpgaTypeStr = CxxUtilities::String::toHexString(fpgaType, 8);
		std::string fpgaVersionStr = CxxUtilities::String::toHexString(fpgaVersion, 8);
		std::string creationDate = CxxUtilities::Time::getCurrentTimeYYYYMMDD_HHMMSS();
		long NSAMPLES = this->nSamples;
		if ( //
		    // FPGA Type and Version
        fits_update_key_str(outputFile, n = (char*) "FPGATYPE", (char*) fpgaTypeStr.c_str(), "FPGA Type",
        &fitsStatus) || //
        fits_update_key_str(outputFile, n = (char*) "FPGAVERS", (char*) fpgaVersionStr.c_str(), "FPGA Version",
        &fitsStatus) || //
				 //fileCreationDate
        fits_update_key_str(outputFile, n = (char*) "FILEDATE", (char*) creationDate.c_str(), "fileCreationDate",
				&fitsStatus) || //
				//detectorID
				fits_update_key_str(outputFile, n = (char*) "DET_ID", (char*) this->detectorID.c_str(), "detectorID",
						&fitsStatus) || //
				//nSamples
				fits_update_key_lng(outputFile, n = (char*) "NSAMPLES", NSAMPLES, "nSamples", &fitsStatus) || //
				//timeTagResolution
				fits_update_key_lng(outputFile, n = (char*) "TIMERES", GROWTH_FY2015_ADC::TimeTagResolutionInNanoSec,
						"timeTagResolution in nano second", &fitsStatus) || //
				//PHA min/max
				fits_update_key_lng(outputFile, n = (char*) "PHA_MIN", GROWTH_FY2015_ADC::PHAMinimum, "PHA range minimum",
						&fitsStatus) || //
				fits_update_key_lng(outputFile, n = (char*) "PHA_MAX", GROWTH_FY2015_ADC::PHAMaximum, "PHA range maximum",
						&fitsStatus) || //
				//exposure
				fits_update_key_dbl(outputFile, n = (char*) "EXPOSURE", this->exposureInSec, 2,
						"exposure specified via command line", &fitsStatus) //

						) {
			using namespace std;
			cerr << "Error: while updating TTYPE comment for " << n << endl;
			exit(-1);
		}

		//configurationYAML as HISTORY
		if (configurationYAMLFile != "") {
			std::vector<std::string> lines = CxxUtilities::File::getAllLines(configurationYAMLFile);
			for (auto line : lines) {
				line = "YAML-- " + line;
				fits_write_history(outputFile, (char*) line.c_str(), &fitsStatus);
			}
		}
	}

private:
	void reportErrorThenQuitIfError(int fitsStatus, std::string methodName) {
		if (fitsStatus) { //if error
			using namespace std;
			cerr << "Error (" << methodName << "):";
			fits_report_error(stderr, fitsStatus);
			exit(-1);
		}
	}

private:
	const int firstElement = 1;

public:
	/** Fill an entry to the HDU containing GPS Time and FPGA Time Tag.
	 * Length of the data should be the same as GROWTH_FY2015_ADC::LengthOfGPSTimeRegister.
	 * @param[in] buffer buffer containing a GPS Time Register data
	 */
	void fillGPSTime(uint8_t* gpsTimeRegisterBuffer) {
		// 0123456789X123456789
		// GPYYMMDDHHMMSSxxxxxx
		long long timeTag = 0;
		for (size_t i = 14; i < GROWTH_FY2015_ADC::LengthOfGPSTimeRegister; i++) {
			timeTag = gpsTimeRegisterBuffer[i] + (timeTag << 8);
		}
		fitsAccessMutes.lock();
		//move to GPS HDU
		fits_movnam_hdu(outputFile, BINARY_TBL, (char*) "GPS", 0, &fitsStatus);
		this->reportErrorThenQuitIfError(fitsStatus, __func__);

		rowIndex_GPS++;

		/* dump date
		 using namespace std;
		 cout << "YY/MM/DD HH:MM:DD = " << gpsTimeRegisterBuffer[2] << gpsTimeRegisterBuffer[3] << "/" //
		 << gpsTimeRegisterBuffer[4] << gpsTimeRegisterBuffer[5] << "/" //
		 << gpsTimeRegisterBuffer[6] << gpsTimeRegisterBuffer[7] << " " //
		 << gpsTimeRegisterBuffer[8] << gpsTimeRegisterBuffer[9] << ":" //
		 << gpsTimeRegisterBuffer[10] << gpsTimeRegisterBuffer[11] << ":" //
		 << gpsTimeRegisterBuffer[12] << gpsTimeRegisterBuffer[13] << endl;
		 */

		//get unix time
		long unixTime = CxxUtilities::Time::getUNIXTimeAsUInt32();

		//write
		fits_write_col(outputFile, TLONGLONG, Column_fpgaTimeTag, rowIndex_GPS, firstElement, 1, &timeTag, &fitsStatus);
		fits_write_col(outputFile, TLONG, Column_unixTime, rowIndex_GPS, firstElement, 1, &unixTime, &fitsStatus);
		fits_write_col(outputFile, TSTRING, Column_gpsTime, rowIndex_GPS, firstElement, 1 /* YYMMDDHHMMSS */,
				&gpsTimeRegisterBuffer, &fitsStatus);

		//move back to EVENTS HDU
		fits_movnam_hdu(outputFile, BINARY_TBL, (char*) "EVENTS", 0, &fitsStatus);
		this->reportErrorThenQuitIfError(fitsStatus, __func__);
		fitsAccessMutes.unlock();
	}

public:
	void fillEvents(std::vector<GROWTH_FY2015_ADC_Type::Event*>& events) {
		fitsAccessMutes.lock();
		for (auto& event : events) {
			rowIndex++;
			//ch
			fits_write_col(outputFile, TBYTE, Column_boardIndexAndChannel, rowIndex, firstElement, 1, &event->ch,
					&fitsStatus);
			//timeTag
			fits_write_col(outputFile, TLONGLONG, Column_timeTag, rowIndex, firstElement, 1, &event->timeTag, &fitsStatus);
			//triggerCount
			fits_write_col(outputFile, TUSHORT, Column_triggerCount, rowIndex, firstElement, 1, &event->triggerCount,
					&fitsStatus);
			//phaMax
			fits_write_col(outputFile, TUSHORT, Column_phaMax, rowIndex, firstElement, 1, &event->phaMax, &fitsStatus);
			//phaMaxTime
			fits_write_col(outputFile, TUSHORT, Column_phaMaxTime, rowIndex, firstElement, 1, &event->phaMaxTime,
					&fitsStatus);
			//phaMin
			fits_write_col(outputFile, TUSHORT, Column_phaMin, rowIndex, firstElement, 1, &event->phaMin, &fitsStatus);
			//phaFirst
			fits_write_col(outputFile, TUSHORT, Column_phaFirst, rowIndex, firstElement, 1, &event->phaFirst, &fitsStatus);
			//phaLast
			fits_write_col(outputFile, TUSHORT, Column_phaLast, rowIndex, firstElement, 1, &event->phaLast, &fitsStatus);
			//maxDerivative
			fits_write_col(outputFile, TUSHORT, Column_maxDerivative, rowIndex, firstElement, 1, &event->maxDerivative,
					&fitsStatus);
			//baseline
			fits_write_col(outputFile, TUSHORT, Column_baseline, rowIndex, firstElement, 1, &event->baseline, &fitsStatus);
			if (nSamples != 0) {
				//waveform
				fits_write_col(outputFile, TUSHORT, Column_waveform, rowIndex, firstElement, nSamples, event->waveform,
						&fitsStatus);
			}

			//
			expandIfNecessary();
		}
		fitsAccessMutes.unlock();
	}

private:
	void expandIfNecessary() {
		using namespace std;
		int fitsStatus = 0;
		//check heap size, and expand row size if necessary (to avoid slow down of cfitsio)
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

public:
	size_t getEntries() {
		return rowIndex;
	}

public:
	void close() {
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
				fits_update_key(outputFile, TULONG, (char*) "NAXIS2", (void*) &rowIndex, (char*) "number of rows in table",
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
};

#endif /* EVENTLISTFILEFITS_HH_ */
