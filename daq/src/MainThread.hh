#ifndef SRC_MAINTHREAD_HH_
#define SRC_MAINTHREAD_HH_

#ifdef USE_ROOT
#include "EventListFileROOT.hh"
#include "TFile.h"
#include "TH1D.h"
#endif

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <thread>

#include "EventListFileFITS.hh"
#include "adcboard.hh"
#include "picojson.h"
#include "timeutil.hh"

//#define DRAW_CANVAS 0

enum class DAQStatus {
  Paused = 0,  //
  Running = 1
};

class MainThread {
 public:
  MainThread(std::string deviceName, std::string configurationFile, u32 exposureInSec)
      : deviceName_(std::move(deviceName)),
        exposureInSec_(exposureInSec),
        configurationFile_(std::move(configurationFile)),
        switchOutputFile_(false) {}
  void start() {
    if (thread_.joinable()) {
      throw std::runtime_error("started twice");
    }
    thread_ = std::thread(&MainThread::run, this);
  }
  void stop() { stopped_ = true; }
  void join() { thread_.join(); }
  void run() {
    setDAQStatus(DAQStatus::Running);
    adcBoard_.reset(new GROWTH_FY2015_ADC(deviceName_));

    fpgaType_ = adcBoard_->getFPGAType();
    fpgaVersion_ = adcBoard_->getFPGAVersion();
    setWaitDurationBetweenEventRead();

    //---------------------------------------------
    // Load configuration file
    //---------------------------------------------
    if (!std::filesystem::exists(configurationFile_)) {
      std::cerr << "Error: YAML configuration file " << configurationFile_ << " not found." << std::endl;
      ::exit(-1);
    }
    adcBoard_->loadConfigurationFile(configurationFile_);

    std::cout << "//---------------------------------------------" << std::endl  //
              << "// Start acquisition" << std::endl                             //
              << "//---------------------------------------------" << std::endl;
    try {
      adcBoard_->startAcquisition();
      std::cout << "Acquisition started." << std::endl;
    } catch (...) {
      std::cerr << "Failed to start acquisition." << std::endl;
      ::exit(-1);
    }

    //---------------------------------------------
    // Create an output file
    //---------------------------------------------
    openOutputEventListFile();

    //---------------------------------------------
    // Send CPU Trigger
    //---------------------------------------------
    std::cout << "Sending CPU Trigger" << std::endl;
    adcBoard_->sendCPUTrigger();

    //---------------------------------------------
    // Read raw ADC values
    //---------------------------------------------
    for (size_t i = 0; i < 4; i++) {
      std::cout << "Ch." << i << " ADC ";
      for (size_t o = 0; o < 5; o++) {
        std::cout << adcBoard_->getCurrentADCValue(i) << " " << std::endl;
      }
    }

    //---------------------------------------------
    // Read GPS Register
    //---------------------------------------------
    std::cout << "Reading GPS Register" << std::endl;
    std::cout << adcBoard_->getGPSRegister() << std::endl;
    readAnsSaveGPSRegister();

    startTime_ = std::chrono::system_clock::now();

    //---------------------------------------------
    // Read events
    //---------------------------------------------
    auto elapsedTime = std::chrono::system_clock::now() - std::chrono::system_clock::now();
    size_t nReceivedEvents = 0;
    stopped_ = false;
    while (!stopped_) {
      nReceivedEvents = readAndThenSaveEvents();
      if (nReceivedEvents == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(eventReadWaitDuration));
      }
      // Get current UNIX time
      const auto currentTime = std::chrono::system_clock::now();
      // Read GPS register if necessary
      if (currentTime - timeOfLastGPSRegisterRead > GPSRegisterReadWaitInSec) {
        readAnsSaveGPSRegister();
      }
      // Update elapsed time
      elapsedTime = currentTime - startTime_;
      // Check whether specified exposure has been completed
      if (exposureInSec_ != exposureInSec_.zero() && elapsedTime >= exposureInSec_) {
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
    std::cout << "Saving event list" << std::endl;

    // Close output file
    closeOutputEventListFile();

    setDAQStatus(DAQStatus::Paused);
  }

  DAQStatus getDAQStatus() const { return daqStatus_; }
  size_t getNEvents() const { return nEvents_; }
  size_t getNEventsOfCurrentOutputFile() const { return nEventsOfCurrentOutputFile_; }
  std::chrono::seconds getElapsedTime() const {
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - startTime_);
  }
  std::chrono::seconds getElapsedTimeOfCurrentOutputFile() const {
    const auto duration = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() -
                                                                           startTimeOfCurrentOutputFile_);
    return (daqStatus_ == DAQStatus::Running) ? duration : std::chrono::seconds{};
  }

  std::string getOutputFileName() const { return outputFileName_.empty() ? "None" : outputFileName_; }

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

  void readAnsSaveGPSRegister() {
    eventListFile_->fillGPSTime(adcBoard_->getGPSRegisterUInt8());
    timeOfLastGPSRegisterRead = std::chrono::system_clock::now();
  }

  /** Sets a wait time duration in millisecond.
   * If the GROWTH_DAQ_WAIT_DURATION environment variable is used, its value is used. Otherwise, the default
   * value is used.
   */
  void setWaitDurationBetweenEventRead() {
    const char* envPointer = std::getenv("GROWTH_DAQ_WAIT_DURATION");
    if (envPointer) {
      const u32 waitDurationInMillisec = atoi(envPointer);
      if (waitDurationInMillisec != 0) {
        eventReadWaitDuration = std::chrono::milliseconds{waitDurationInMillisec};
        return;
      }
    }
    eventReadWaitDuration = DefaultEventReadWaitDuration;
  }

  void openOutputEventListFile() {
    startTimeOfCurrentOutputFile_ = std::chrono::system_clock::now();
    nEventsOfCurrentOutputFile_ = 0;
#ifdef USE_ROOT
    outputFileName_ = timeutil::getCurrentTimeYYYYMMDD_HHMMSS() + ".root";
    eventListFile_.reset(new EventListFileROOT(outputFileName_, adcBoard_->DetectorID, configurationFile_));
#else
    outputFileName_ = timeutil::getCurrentTimeYYYYMMDD_HHMMSS() + ".fits";
    eventListFile_.reset(new EventListFileFITS(outputFileName_, adcBoard_->DetectorID, configurationFile_,       //
                                               adcBoard_->getNSamplesInEventListFile(), exposureInSec_.count(),  //
                                               fpgaType_, fpgaVersion_));
#endif
    std::cout << "Output file name: " << outputFileName_ << std::endl;
  }

  void closeOutputEventListFile() {
    if (eventListFile_) {
      eventListFile_->close();
      eventListFile_.reset();
      outputFileName_ = {};
      startTimeOfCurrentOutputFile_ = {};
    }
  }

  size_t readAndThenSaveEvents() {
    // Start recording to a new file is ordered.
    if (switchOutputFile_) {
      closeOutputEventListFile();
      openOutputEventListFile();
      switchOutputFile_ = false;
    }

    std::vector<GROWTH_FY2015_ADC_Type::Event*> events = adcBoard_->getEvent();
    std::cout << "Received " << events.size() << " events" << std::endl;
    eventListFile_->fillEvents(events);

    const size_t nReceivedEvents = events.size();
    nEvents_ += nReceivedEvents;
    nEventsOfCurrentOutputFile_ += nReceivedEvents;
    std::cout << events.size() << " events (" << nEvents_ << ")" << std::endl;
    adcBoard_->freeEvents(events);

    return nReceivedEvents;
  }

  const std::string deviceName_;
  const std::string configurationFile_;
  const std::chrono::seconds exposureInSec_{};

  std::atomic<bool> stopped_{true};
  std::atomic<bool> hasStopped_{};

  const std::chrono::milliseconds DefaultEventReadWaitDuration = std::chrono::milliseconds(50);
  std::chrono::milliseconds eventReadWaitDuration = DefaultEventReadWaitDuration;
  const std::chrono::seconds GPSRegisterReadWaitInSec = std::chrono::seconds(30);
  std::chrono::system_clock::time_point timeOfLastGPSRegisterRead{};

  std::unique_ptr<GROWTH_FY2015_ADC> adcBoard_;
  u32 fpgaType_{};
  u32 fpgaVersion_{};
  size_t nEvents_ = 0;
  size_t nEventsOfCurrentOutputFile_ = 0;
#ifdef USE_ROOT
  std::unique_ptr<EventListFileROOT> eventListFile_;
#else
  std::unique_ptr<EventListFileFITS> eventListFile_;
#endif

  std::chrono::system_clock::time_point startTime_{};
  std::chrono::system_clock::time_point startTimeOfCurrentOutputFile_{};

  std::string outputFileName_{};
  std::atomic<bool> switchOutputFile_{};
  DAQStatus daqStatus_ = DAQStatus::Paused;
  std::mutex daqStatusMutex;

  std::thread thread_{};
};

#endif /* SRC_MAINTHREAD_HH_ */
