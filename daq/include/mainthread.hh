#ifndef GROWTHDAQ_MAINTHREAD_HH_
#define GROWTHDAQ_MAINTHREAD_HH_

#ifdef USE_ROOT
#include "eventlistfileroot.hh"
#include "TFile.h"
#include "TH1D.h"
#endif

#include "eventlistfilefits.hh"
#include "adcboard.hh"
#include "picojson.h"
#include "timeutil.hh"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <thread>

enum class DAQStatus {
  Paused = 0,  //
  Running = 1
};

class MainThread {
 public:
  MainThread(std::string deviceName, std::string configurationFile, u32 exposureInSec);
  void start();
  void stop();
  void join();
  void run();
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
  void startNewOutputFile();

  /** Reads a 16-bit word from the specified address. */
  std::optional<u16> readRegister16(const u32 address) { return adcBoard_->readRegister16(address); }

 private:
  void setDAQStatus(DAQStatus status);
  void readAnsSaveGPSRegister();
  /** Sets a wait time duration in millisecond.
   * If the GROWTH_DAQ_WAIT_DURATION environment variable is used, its value is used. Otherwise, the default
   * value is used.
   */
  void setWaitDurationBetweenEventRead();
  void openOutputEventListFile();
  void closeOutputEventListFile();
  size_t readAndThenSaveEvents();

  const std::string deviceName_;
  const std::string configurationFile_;
  const std::chrono::seconds exposureInSec_{};

  std::atomic<bool> stopped_{true};
  std::atomic<bool> hasStopped_{};

  const std::chrono::milliseconds DEFAULT_EVENT_READ_WAIT_DURATION = std::chrono::milliseconds(50);
  std::chrono::milliseconds eventReadWaitDuration = DEFAULT_EVENT_READ_WAIT_DURATION;
  const std::chrono::milliseconds GPSRegisterReadWaitInSec = std::chrono::milliseconds(2000);
  std::chrono::system_clock::time_point timeOfLastGPSRegisterRead_{};

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
  std::mutex daqStatusMutex_;

  std::thread thread_{};
};

#endif /* SRC_MAINTHREAD_HH_ */
