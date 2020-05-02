/*
 * test_growth_fy2015_adc_cpuTrigger.cc
 *
 *  Created on: Jun 10, 2015
 *      Author: yuasa
 */

#include "GROWTH_FY2015_ADC.hh"
#include "TApplication.h"
#include "TCanvas.h"
#include "TFile.h"
#include "TH1D.h"
#include "TROOT.h"

static const uint32_t AddressOf_EventFIFO_DataCountRegister = 0x20000000;

#define DRAW_CANVAS 1

class MainThread : public CxxUtilities::StoppableThread {
 public:
  std::string deviceName_;
  size_t nEventsMax;
  TApplication* app;

 public:
  MainThread(std::string deviceName, size_t nEventsMax, TApplication* app) {
    this->deviceName_ = deviceName;
    this->nEventsMax = nEventsMax;
    this->app        = app;
  }

 public:
  void run() {
    using namespace std;
    auto adcBoard                           = new GROWTH_FY2015_ADC(deviceName_);
    const size_t nChannels                  = 4;
    const size_t NumberOfSamples            = 1000;
    const size_t NumberOfSamplesEventPacket = 2;
    const size_t PreTriggerSamples          = 4;
    const size_t TriggerThresholds[1][4]    = {{900, 530, 900, 900}};

    std::vector<size_t> enabledChannels = {0, 1, 2, 3};
    uint16_t ChannelEnableMask          = 0x0F;  // enable all 4 channels

#ifdef DRAW_CANVAS
    //---------------------------------------------
    // Run ROOT eventloop
    //---------------------------------------------
    TCanvas* canvas = new TCanvas("c", "c", 500, 500);
    canvas->Draw();
    canvas->SetLogy();
#endif

    //---------------------------------------------
    // Program the digitizer
    //---------------------------------------------
    try {
      // record legnth
      adcBoard->setNumberOfSamples(NumberOfSamples);
      adcBoard->setNumberOfSamplesInEventPacket(NumberOfSamplesEventPacket);

      for (auto ch : enabledChannels) {
        // pre-trigger (delay)
        adcBoard->setDepthOfDelay(ch, PreTriggerSamples);

        // trigger mode
        adcBoard->setTriggerMode(ch, SpaceFibreADC::TriggerMode::StartThreshold_NSamples_CloseThreshold);

        // threshold
        adcBoard->setStartingThreshold(ch, TriggerThresholds[0][ch]);
        adcBoard->setClosingThreshold(ch, TriggerThresholds[0][ch]);

        // adc clock 50MHz
        adcBoard->setAdcClock(SpaceFibreADC::ADCClockFrequency::ADCClock50MHz);

        // turn on ADC
        adcBoard->turnOnADCPower(ch);
      }
      cout << "Device configurtion done." << endl;
    } catch (...) {
      cerr << "Device configuration failed." << endl;
      ::exit(-1);
    }

    //---------------------------------------------
    // Start acquisition
    //---------------------------------------------
    std::vector<bool> startStopRegister(nChannels);
    uint16_t mask = 0x0001;
    for (size_t i = 0; i < nChannels; i++) {
      if ((ChannelEnableMask & mask) == 0) {
        startStopRegister[i] = false;
      } else {
        startStopRegister[i] = true;
      }
      mask = mask << 1;
    }
    try {
      adcBoard->startAcquisition(startStopRegister);
      cout << "Acquisition started." << endl;
    } catch (...) {
      cerr << "Failed to start acquisition." << endl;
      ::exit(-1);
    }

    //---------------------------------------------
    // Send CPU Trigger
    //---------------------------------------------
    for (auto ch : enabledChannels) {
      cout << "Sending CPU Trigger to Channel " << ch << endl;
      adcBoard->sendCPUTrigger(ch);
    }

    //---------------------------------------------
    // Read status
    //---------------------------------------------
    ChannelModule* channelModule = adcBoard->getChannelRegister(1);
    printf("Livetime Ch.1 = %d\n", channelModule->getLivetime());
    printf("ADC Ch.1 = %d\n", channelModule->getCurrentADCValue());
    cout << channelModule->getStatus() << endl;

    size_t eventFIFODataCount = adcBoard->getRMAPHandler()->getRegister(AddressOf_EventFIFO_DataCountRegister);
    printf("EventFIFO data count = %zu\n", eventFIFODataCount);
    printf("Trigger count = %zu\n", channelModule->getTriggerCount());
    printf("ADC Ch.1 = %d\n", channelModule->getCurrentADCValue());

    //---------------------------------------------
    // Read events
    //---------------------------------------------
    size_t nEvents = 0;
    TH1D* hist     = new TH1D("h", "Histogram", 1024, 0, 1024);
    CxxUtilities::Condition c;

#ifdef DRAW_CANVAS
    hist->GetXaxis()->SetRangeUser(480, 1024);
    hist->GetXaxis()->SetTitle("ADC Channel");
    hist->GetYaxis()->SetTitle("Counts");
    hist->Draw();
    canvas->Update();
    //	RootEventLoop eventloop(app);
    //	eventloop.start();

    size_t canvasUpdateCounter          = 0;
    const size_t canvasUpdateCounterMax = 10;
#endif

    while (nEvents < nEventsMax) {
      std::vector<GROWTH_FY2015_ADC_Type::Event*> events = adcBoard->getEvent();
      cout << "Received " << events.size() << " events" << endl;
      for (auto event : events) {
        /*
         cout << (uint32_t) event->ch << endl;
         for (size_t i = 0; i < event->nSamples; i++) {
         cout << dec << (uint32_t) event->waveform[i] << " ";
         }
         cout << dec << endl;
         */
        hist->Fill(event->phaMax);
      }
      nEvents += events.size();
      cout << events.size() << " events (" << nEvents << ")" << endl;
      adcBoard->freeEvents(events);
      c.wait(100);
#ifdef DRAW_CANVAS
      canvasUpdateCounter++;
      if (canvasUpdateCounter == canvasUpdateCounterMax) {
        cout << "Update canvas." << endl;
        canvasUpdateCounter = 0;
        hist->Draw();
        canvas->Update();
      }

#endif
    }

    cout << "Saving histogram" << endl;
    TFile* file = new TFile("hist.root", "recreate");
    hist->Write();
    file->Close();

    adcBoard->stopAcquisition();
    adcBoard->closeDevice();
    cout << "Waiting child threads to be finalized..." << endl;
    c.wait(1000);
    cout << "Deleting ADCBoard instance." << endl;
    delete adcBoard;
    cout << "Terminating ROOT event loop." << endl;

#ifdef DRAW_CANVAS
    canvas->Close();
    delete canvas;
#endif
    app->Terminate(0);
    gROOT->ProcessLine(".q");
  }
};

int main(int argc, char* argv[]) {
  using namespace std;
  if (argc < 3) {
    cerr << "Provide UART device name (e.g. /dev/tty.usb-aaa-bbb) and number of event to be collected." << endl;
    ::exit(-1);
  }
  std::string deviceName(argv[1]);
  size_t nEventsMax = atoi(argv[2]);
  TApplication* app = new TApplication("app", &argc, argv);

  MainThread* mainThread = new MainThread(deviceName, nEventsMax, app);
  mainThread->start();

  app->Run(true);
  return 0;
}
