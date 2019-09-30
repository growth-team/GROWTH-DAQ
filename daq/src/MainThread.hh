#ifndef SRC_MAINTHREAD_HH_
#define SRC_MAINTHREAD_HH_

#ifdef USE_ROOT
#include "EventListFileROOT.hh"
#include "TFile.h"
#include "TH1D.h"
#endif
#ifdef DRAW_CANVAS
#include "TApplication.h"
#include "TCanvas.h"
#include "TFile.h"
#include "TH1D.h"
#include "TROOT.h"
TApplication* app;
#endif

#include <cstdlib>
#include "EventListFileFITS.hh"
#include "GROWTH_FY2015_ADC.hh"

//#define DRAW_CANVAS 0

enum class DAQStatus {
  Paused  = 0,  //
  Running = 1
};

class MainThread : public CxxUtilities::StoppableThread {
 public:
  std::string deviceName;
  std::string configurationFile;
  double exposureInSec;

 public:
  MainThread(std::string deviceName, std::string configurationFile, double exposureInSec) {
    this->deviceName        = deviceName;
    this->exposureInSec     = exposureInSec;
    this->configurationFile = configurationFile;
    this->switchOutputFile  = false;
    setDAQStatus(DAQStatus::Paused);
  }

 public:
  void run() {
    using namespace std;
    setDAQStatus(DAQStatus::Running);
    switchOutputFile = false;
    adcBoard         = new GROWTH_FY2015_ADC(deviceName);

    fpgaType    = adcBoard->getFPGAType();
    fpgaVersion = adcBoard->getFPGAVersion();
    setWaitDurationBetweenEventRead();

#ifdef DRAW_CANVAS
    //---------------------------------------------
    // Run ROOT event loop
    //---------------------------------------------
    canvas = new TCanvas("c", "c", 500, 500);
    canvas->Draw();
    canvas->SetLogy();
#endif

    //---------------------------------------------
    // Load configuration file
    //---------------------------------------------
    if (!CxxUtilities::File::exists(configurationFile)) {
      cerr << "Error: YAML configuration file " << configurationFile << " not found." << endl;
      ::exit(-1);
    }
    adcBoard->loadConfigurationFile(configurationFile);

    cout << "//---------------------------------------------" << endl  //
         << "// Start acquisition" << endl                             //
         << "//---------------------------------------------" << endl;
    try {
      adcBoard->startAcquisition();
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
    adcBoard->sendCPUTrigger();

    //---------------------------------------------
    // Read raw ADC values
    //---------------------------------------------
    for (size_t i = 0; i < 4; i++) {
      cout << "Ch." << i << " ADC ";
      for (size_t o = 0; o < 5; o++) { cout << (uint32_t)adcBoard->getCurrentADCValue(i) << " " << endl; }
    }

    //---------------------------------------------
    // Read GPS Register
    //---------------------------------------------
    cout << "Reading GPS Register" << endl;
    cout << adcBoard->getGPSRegister() << endl;
    readAnsSaveGPSRegister();

    //---------------------------------------------
    // Log start time
    //---------------------------------------------
    startUnixTime = CxxUtilities::Time::getUNIXTimeAsUInt32();

    //---------------------------------------------
    // Read events
    //---------------------------------------------
#ifdef DRAW_CANVAS
    hist = new TH1D("h", "Histogram", 1024, 0, 1024);
    hist->GetXaxis()->SetRangeUser(480, 1024);
    hist->GetXaxis()->SetTitle("ADC Channel");
    hist->GetYaxis()->SetTitle("Counts");
    hist->Draw();
    canvas->Update();
    canvasUpdateCounter = 0;
#endif

    uint32_t elapsedTime   = 0;
    size_t nReceivedEvents = 0;
    stopped                = false;
    while (!stopped) {
      nReceivedEvents = readAndThenSaveEvents();
      if (nReceivedEvents == 0) { c.wait(eventReadWaitDuration); }
      // Get current UNIX time
      uint32_t currentUnixTime = CxxUtilities::Time::getUNIXTimeAsUInt32();
      // Read GPS register if necessary
      if (currentUnixTime - unixTimeOfLastGPSRegisterRead > GPSRegisterReadWaitInSec) { readAnsSaveGPSRegister(); }
      // Update elapsed time
      elapsedTime = currentUnixTime - startUnixTime;
      // Check whether specified exposure has been completed
      if (exposureInSec > 0 && elapsedTime >= exposureInSec) { break; }
    }

#ifdef DRAW_CANVAS
    cout << "Saving histogram" << endl;
    TFile* file = new TFile("histogram.root", "recreate");
    file->cd();
    hist->Write();
    file->Close();
#endif

    //---------------------------------------------
    // Finalize observation run
    //---------------------------------------------

    // Stop acquisition first
    adcBoard->stopAcquisition();

    // Completely read the EventFIFO
    readAndThenSaveEvents();
    readAndThenSaveEvents();
    readAndThenSaveEvents();
    cout << "Saving event list" << endl;

    // Close output file
    closeOutputEventListFile();

    // FIanlize the board
    adcBoard->closeDevice();
    cout << "Waiting child threads to be finalized..." << endl;
    c.wait(1000);
    cout << "Deleting ADCBoard instance." << endl;
    delete adcBoard;
    setDAQStatus(DAQStatus::Paused);
  }

 public:
  DAQStatus getDAQStatus() const { return daqStatus; }

 public:
  const size_t getNEvents() const { return nEvents; }

 public:
  const size_t getNEventsOfCurrentOutputFile() const { return nEventsOfCurrentOutputFile; }

 public:
  const size_t getElapsedTime() const {
    uint32_t currentUnixTime = CxxUtilities::Time::getUNIXTimeAsUInt32();
    return currentUnixTime - startUnixTime;
  }

 public:
  const size_t getElapsedTimeOfCurrentOutputFile() const {
    if (daqStatus == DAQStatus::Running) {
      uint32_t currentUnixTime = CxxUtilities::Time::getUNIXTimeAsUInt32();
      return currentUnixTime - startUnixTimeOfCurrentOutputFile;
    } else {
      return 0;
    }
  }

 public:
  const std::string getOutputFileName() const {
    if (outputFileName != "") {
      return outputFileName;
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
  void startNewOutputFile() { switchOutputFile = true; }

 private:
  void debug_readStatus(int debugChannel = 3) {
    using namespace std;
    //---------------------------------------------
    // Read status
    //---------------------------------------------
    ChannelModule* channelModule = adcBoard->getChannelRegister(debugChannel);
    printf("Debugging Ch.%d\n", debugChannel);
    printf("ADC          = %d\n", channelModule->getCurrentADCValue());
    printf("Livetime     = %d\n", channelModule->getLivetime());
    cout << channelModule->getStatus() << endl;
    size_t eventFIFODataCount = adcBoard->getRMAPHandler()->getRegister(  //
        ConsumerManagerEventFIFO::AddressOf_EventFIFO_DataCount_Register);
    printf("EventFIFO Count = %zu\n", eventFIFODataCount);
    printf("TriggerCount = %zu\n", channelModule->getTriggerCount());
    printf("ADC          = %d\n", channelModule->getCurrentADCValue());
  }

 private:
  void setDAQStatus(DAQStatus status) {
    daqStatusMutex.lock();
    daqStatus = status;
    daqStatusMutex.unlock();
  }

 private:
  void readAnsSaveGPSRegister() {
    eventListFile->fillGPSTime(adcBoard->getGPSRegisterUInt8());
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
      uint32_t waitDurationInMillisec = atoi(envPointer);
      if (waitDurationInMillisec != 0) { eventReadWaitDuration = waitDurationInMillisec; }
    }
    eventReadWaitDuration = DefaultEventReadWaitDurationInMillisec;
  }

 private:
  void openOutputEventListFile() {
    startUnixTimeOfCurrentOutputFile = CxxUtilities::Time::getUNIXTimeAsUInt32();
    nEventsOfCurrentOutputFile       = 0;
#ifdef USE_ROOT
    outputFileName = CxxUtilities::Time::getCurrentTimeYYYYMMDD_HHMMSS() + ".root";
    eventListFile  = new EventListFileROOT(outputFileName, adcBoard->DetectorID, configurationFile);
#else
    outputFileName = CxxUtilities::Time::getCurrentTimeYYYYMMDD_HHMMSS() + ".fits";
    eventListFile  = new EventListFileFITS(outputFileName, adcBoard->DetectorID, configurationFile,  //
                                          adcBoard->getNSamplesInEventListFile(), exposureInSec,    //
                                          fpgaType, fpgaVersion);
#endif
    std::cout << "Output file name: " << outputFileName << std::endl;
  }

 private:
  void closeOutputEventListFile() {
    if (eventListFile != nullptr) {
      eventListFile->close();
      delete eventListFile;
      eventListFile                    = nullptr;
      outputFileName                   = "";
      startUnixTimeOfCurrentOutputFile = 0;
    }
  }

 private:
  size_t readAndThenSaveEvents() {
    using namespace std;

    // Start recording to a new file is ordered.
    if (switchOutputFile) {
      closeOutputEventListFile();
      openOutputEventListFile();
      switchOutputFile = false;
    }

    std::vector<GROWTH_FY2015_ADC_Type::Event*> events = adcBoard->getEvent();
    cout << "Received " << events.size() << " events" << endl;
    eventListFile->fillEvents(events);

#ifdef DRAW_CANVAS
    cout << "Filling to histogram" << endl;
    for (auto event : events) { hist->Fill(event->phaMax); }
#endif

    size_t nReceivedEvents = events.size();
    nEvents += nReceivedEvents;
    nEventsOfCurrentOutputFile += nReceivedEvents;
    cout << events.size() << " events (" << nEvents << ")" << endl;
    adcBoard->freeEvents(events);

#ifdef DRAW_CANVAS
    canvasUpdateCounter++;
    if (canvasUpdateCounter == canvasUpdateCounterMax) {
      cout << "Update canvas." << endl;
      canvasUpdateCounter = 0;
      hist->Draw();
      canvas->Update();
    }
#endif

    return nReceivedEvents;
  }

 private:
  static const uint32_t DefaultEventReadWaitDurationInMillisec = 50;
  uint32_t eventReadWaitDuration                               = DefaultEventReadWaitDurationInMillisec;
  static const size_t GPSRegisterReadWaitInSec                 = 30;  // 30s
  uint32_t unixTimeOfLastGPSRegisterRead                       = 0;

 private:
  GROWTH_FY2015_ADC* adcBoard;
  CxxUtilities::Condition c;
  uint32_t fpgaType;
  uint32_t fpgaVersion;
  size_t nEvents                    = 0;
  size_t nEventsOfCurrentOutputFile = 0;
#ifdef DRAW_CANVAS
  TCanvas* canvas;
  TH1D* hist;
  size_t canvasUpdateCounter;
  const size_t canvasUpdateCounterMax = 10;
#endif
#ifdef USE_ROOT
  EventListFileROOT* eventListFile = nullptr;
#else
  EventListFileFITS* eventListFile = nullptr;
#endif

 private:
  uint32_t startUnixTime;
  uint32_t startUnixTimeOfCurrentOutputFile;
  std::string outputFileName;
  bool switchOutputFile;
  DAQStatus daqStatus;
  CxxUtilities::Mutex daqStatusMutex;
};

#endif /* SRC_MAINTHREAD_HH_ */
