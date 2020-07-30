#ifndef SRC_MAINTHREAD_HH_
#define SRC_MAINTHREAD_HH_

#ifdef USE_ROOT
#include "EventListFileROOT.hh"
#include "TFile.h"
#include "TH1D.h"
#endif

#include <cstdlib>
#include "EventListFileFITS.hh"
#include "adcboard.hh"

#include <chrono>
#include <thread>

//#define DRAW_CANVAS 0

enum class DAQStatus {
  Paused = 0,  //
  Running = 1
};

class MainThread : public CxxUtilities::StoppableThread {
 public:
  MainThread(std::string deviceName, std::string configurationFile, f64 exposureInSec)
      : deviceName_(deviceName),
        exposureInSec_(exposureInSec),
        configurationFile_(configurationFile),
        switchOutputFile_(false) {}

 public:
  void run() {
    using namespace std;
    setDAQStatus(DAQStatus::Running);
    adcBoard_.reset(new GROWTH_FY2015_ADC(deviceName_));

    fpgaType_ = adcBoard_->getFPGAType();
    fpgaVersion_ = adcBoard_->getFPGAVersion();
    setWaitDurationBetweenEventRead();

    //---------------------------------------------
    // Load configuration file
    //---------------------------------------------
    if (!CxxUtilities::File::exists(configurationFile_)) {
      cerr << "Error: YAML configuration file " << configurationFile_ << " not found." << endl;
      ::exit(-1);
    }
    adcBoard_->loadConfigurationFile(configurationFile_);

    cout << "//---------------------------------------------" << endl  //
         << "// Start acquisition" << endl                             //
         << "//---------------------------------------------" << endl;
    try {
      adcBoard_->startAcquisition();
      cout << "Acquisition started." << endl;
    } catch (...) {
      cerr << "Failed to start acquisition." << endl;
      ::exit(-1);
    }

    //---------------------------------------------
    // Create an output file
    //---------------------------------------------
    openOutputEventListFile();

    //---------------------------------------------
    // Send CPU Trigger
    //---------------------------------------------
    cout << "Sending CPU Trigger" << endl;
    adcBoard_->sendCPUTrigger();

    //---------------------------------------------
    // Read raw ADC values
    //---------------------------------------------
    for (size_t i = 0; i < 4; i++) {
      cout << "Ch." << i << " ADC ";
      for (size_t o = 0; o < 5; o++) {
        cout << adcBoard_->getCurrentADCValue(i) << " " << endl;
      }
    }

    //---------------------------------------------
    // Read GPS Register
    //---------------------------------------------
    cout << "Reading GPS Register" << endl;
    cout << adcBoard_->getGPSRegister() << endl;
    readAnsSaveGPSRegister();

    //---------------------------------------------
    // Log start time
    //---------------------------------------------
    startUnixTime_ = CxxUtilities::Time::getUNIXTimeAsUInt32();

    //---------------------------------------------
    // Read events
    //---------------------------------------------
    u32 elapsedTime = 0;
    size_t nReceivedEvents = 0;
    stopped = false;
    while (!stopped) {
      nReceivedEvents = readAndThenSaveEvents();
      if (nReceivedEvents == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(eventReadWaitDuration));
      }
      // Get current UNIX time
      u32 currentUnixTime = CxxUtilities::Time::getUNIXTimeAsUInt32();
      // Read GPS register if necessary
      if (currentUnixTime - unixTimeOfLastGPSRegisterRead > GPSRegisterReadWaitInSec) {
        readAnsSaveGPSRegister();
      }
      // Update elapsed time
      elapsedTime = currentUnixTime - startUnixTime_;
      // Check whether specified exposure has been completed
      if (exposureInSec_ > 0 && elapsedTime >= exposureInSec_) {
        break;
      }
    }

    //---------------------------------------------
    // Finalize observation run
    //---------------------------------------------

    // Stop acquisition first
    adcBoard_->stopAcquisition();

    // Completely read the EventFIFO
    readAndThenSaveEvents();
    readAndThenSaveEvents();
    readAndThenSaveEvents();
    cout << "Saving event list" << endl;

    // Close output file
    closeOutputEventListFile();

    // FIanlize the board
    adcBoard_->closeDevice();
    cout << "Waiting child threads to be finalized..." << endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    setDAQStatus(DAQStatus::Paused);
  }

 public:
  DAQStatus getDAQStatus() const { return daqStatus_; }

 public:
  size_t getNEvents() const { return nEvents_; }

 public:
  size_t getNEventsOfCurrentOutputFile() const { return nEventsOfCurrentOutputFile_; }

 public:
  size_t getElapsedTime() const {
    u32 currentUnixTime = CxxUtilities::Time::getUNIXTimeAsUInt32();
    return currentUnixTime - startUnixTime_;
  }

 public:
  const size_t getElapsedTimeOfCurrentOutputFile() const {
    if (daqStatus_ == DAQStatus::Running) {
      u32 currentUnixTime = CxxUtilities::Time::getUNIXTimeAsUInt32();
      return currentUnixTime - startUnixTimeOfCurrentOutputFile_;
    } else {
      return 0;
    }
  }

 public:
  const std::string getOutputFileName() const {
    if (outputFileName_ != "") {
      return outputFileName_;
    } else {
      return "None";
    }
  }

 public:
  /** This method is called to close the current output event list file,
   * and create a new file with a new time stamp. This method is called by
   * MessageServer class when the class is commanded to split output files.
   * A new file is created when the next event(s) is(are) read from the board.
   */
  void startNewOutputFile() { switchOutputFile_ = true; }

 private:
  void setDAQStatus(DAQStatus status) {
    std::lock_guard<std::mutex> guard(daqStatusMutex);
    daqStatus_ = status;
  }

 private:
  void readAnsSaveGPSRegister() {
    eventListFile_->fillGPSTime(adcBoard_->getGPSRegisterUInt8());
    unixTimeOfLastGPSRegisterRead = CxxUtilities::Time::getUNIXTimeAsUInt32();
  }

 private:
  /** Sets a wait time duration in millisecond.
   * If the GROWTH_DAQ_WAIT_DURATION environment variable is
   * used, its value is used. Otherwise, the default value is
   * used.
   */
  void setWaitDurationBetweenEventRead() {
    const char* envPointer = std::getenv("GROWTH_DAQ_WAIT_DURATION");
    if (envPointer != NULL) {
      const u32 waitDurationInMillisec = atoi(envPointer);
      if (waitDurationInMillisec != 0) {
        eventReadWaitDuration = waitDurationInMillisec;
      }
    }
    eventReadWaitDuration = DefaultEventReadWaitDurationInMillisec;
  }

 private:
  void openOutputEventListFile() {
    startUnixTimeOfCurrentOutputFile_ = CxxUtilities::Time::getUNIXTimeAsUInt32();
    nEventsOfCurrentOutputFile_ = 0;
#ifdef USE_ROOT
    outputFileName_ = CxxUtilities::Time::getCurrentTimeYYYYMMDD_HHMMSS() + ".root";
    eventListFile_ = new EventListFileROOT(outputFileName_, adcBoard_->DetectorID, configurationFile_);
#else
    outputFileName_ = CxxUtilities::Time::getCurrentTimeYYYYMMDD_HHMMSS() + ".fits";
    eventListFile_ = new EventListFileFITS(outputFileName_, adcBoard_->DetectorID, configurationFile_,  //
                                           adcBoard_->getNSamplesInEventListFile(), exposureInSec_,     //
                                           fpgaType_, fpgaVersion_);
#endif
    std::cout << "Output file name: " << outputFileName_ << std::endl;
  }

 private:
  void closeOutputEventListFile() {
    if (eventListFile_ != nullptr) {
      eventListFile_->close();
      delete eventListFile_;
      eventListFile_ = nullptr;
      outputFileName_ = "";
      startUnixTimeOfCurrentOutputFile_ = 0;
    }
  }

 private:
  size_t readAndThenSaveEvents() {
    using namespace std;

    // Start recording to a new file is ordered.
    if (switchOutputFile_) {
      closeOutputEventListFile();
      openOutputEventListFile();
      switchOutputFile_ = false;
    }

    std::vector<GROWTH_FY2015_ADC_Type::Event*> events = adcBoard_->getEvent();
    cout << "Received " << events.size() << " events" << endl;
    eventListFile_->fillEvents(events);

    const size_t nReceivedEvents = events.size();
    nEvents_ += nReceivedEvents;
    nEventsOfCurrentOutputFile_ += nReceivedEvents;
    cout << events.size() << " events (" << nEvents_ << ")" << endl;
    adcBoard_->freeEvents(events);

    return nReceivedEvents;
  }

 private:
  std::string deviceName_;
  std::string configurationFile_;
  f64 exposureInSec_{};

  static constexpr u32 DefaultEventReadWaitDurationInMillisec = 50;
  u32 eventReadWaitDuration = DefaultEventReadWaitDurationInMillisec;
  static constexpr size_t GPSRegisterReadWaitInSec = 30;  // 30s
  u32 unixTimeOfLastGPSRegisterRead = 0;

  std::unique_ptr<GROWTH_FY2015_ADC> adcBoard_;
  u32 fpgaType_{};
  u32 fpgaVersion_{};
  size_t nEvents_ = 0;
  size_t nEventsOfCurrentOutputFile_ = 0;
#ifdef USE_ROOT
  EventListFileROOT* eventListFile_ = nullptr;
#else
  EventListFileFITS* eventListFile_ = nullptr;
#endif

  u32 startUnixTime_{};
  u32 startUnixTimeOfCurrentOutputFile_{};
  std::string outputFileName_{};
  bool switchOutputFile_{};
  DAQStatus daqStatus_ = DAQStatus::Paused;
  std::mutex daqStatusMutex;
};

#endif /* SRC_MAINTHREAD_HH_ */
