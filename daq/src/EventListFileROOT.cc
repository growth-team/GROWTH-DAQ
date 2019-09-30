#include "EventListFileROOT.hh"

  EventListFileROOT::EventListFileROOT(std::string fileName, std::string detectorID = "empty", std::string configurationYAMLFile = "")
      : EventListFile(fileName), detectorID(detectorID), configurationYAMLFile(configurationYAMLFile) {
    eventEntry.waveform = new uint16_t[SpaceFibreADC::MaxWaveformLength];
    createOutputRootFile();
  }

  EventListFileROOT::~EventListFileROOT() {
    close();
    delete eventEntry.waveform;
  }

  void EventListFileROOT::fillEvents(std::vector<GROWTH_FY2015_ADC_Type::Event*>& events) {
    unixTime = CxxUtilities::Time::getUNIXTimeAsUInt32();
    for (auto& event : events) {
      copyEventData(event, &eventEntry);
      eventTree->Fill();
    }
  }

  size_t EventListFileROOT::getEntries() { return eventTree->GetEntries(); }

  void EventListFileROOT::fillGPSTime(uint8_t* gpsTimeRegisterBuffer) {}

  void EventListFileROOT::close() {
    if (outputFile != NULL) {
      eventTree->Write();
      outputFile->Close();
    }
  }

  void EventListFileROOT::createOutputRootFile() {
    outputFile.reset(new TFile(fileName.c_str(), "recreate"));
    eventTree.reset(new TTree("eventTree", "eventTree"));

    // - C : a character string terminated by the 0 character
    // - B : an 8 bit signed integer (Char_t)
    // - b : an 8 bit unsigned integer (UChar_t)
    // - S : a 16 bit signed integer (Short_t)
    // - s : a 16 bit unsigned integer (UShort_t)
    // - I : a 32 bit signed integer (Int_t)
    // - i : a 32 bit unsigned integer (UInt_t)
    // - F : a 32 bit floating point (Float_t)
    // - D : a 64 bit floating point (Double_t)
    // - L : a 64 bit signed integer (Long64_t)
    // - l : a 64 bit unsigned integer (ULong64_t)
    // - O : [the letter 'o', not a zero] a boolean (Bool_t)

    eventTree->Branch("boardIndexAndChannel", &eventEntry.ch, "boardIndexAndChannel/b");
    eventTree->Branch("timeTag", &eventEntry.timeTag, "timeTag/l");
    eventTree->Branch("unixTime", &unixTime, "unixTime/i");
    eventTree->Branch("triggerCount", &eventEntry.triggerCount, "triggerCount/s");
    eventTree->Branch("nSamples", &eventEntry.nSamples, "nSamples/s");
    eventTree->Branch("phaMax", &eventEntry.phaMax, "phaMax/s");
    eventTree->Branch("phaMaxTime", &eventEntry.phaMax, "phaMaxTime/s");
    eventTree->Branch("phaMin", &eventEntry.phaMax, "phaMin/s");
    eventTree->Branch("phaFirst", &eventEntry.phaMax, "phaFirst/s");
    eventTree->Branch("phaLast", &eventEntry.phaMax, "phaLast/s");
    eventTree->Branch("maxDerivative", &eventEntry.phaMax, "maxDerivative/s");
    eventTree->Branch("baseline", &eventEntry.phaMax, "baseline/s");
    eventTree->Branch("waveform", eventEntry.waveform, "waveform[nSamples]/s");

    writeHeader();
  }

  void EventListFileROOT::copyEventData(GROWTH_FY2015_ADC_Type::Event* from, GROWTH_FY2015_ADC_Type::Event* to) {
    to->ch            = from->ch;
    to->timeTag       = from->timeTag;
    to->triggerCount  = from->triggerCount;
    to->phaMax        = from->phaMax;
    to->phaMaxTime    = from->phaMaxTime;
    to->phaMin        = from->phaMin;
    to->phaFirst      = from->phaFirst;
    to->phaLast       = from->phaLast;
    to->maxDerivative = from->maxDerivative;
    to->baseline      = from->baseline;
    to->nSamples      = from->nSamples;
    memcpy(to->waveform, from->waveform, sizeof(uint16_t) * from->nSamples);
  }

  void EventListFileROOT::writeHeader() {
    CxxUtilities::ROOTUtilities::writeObjString("fileCreationDate",
                                                CxxUtilities::Time::getCurrentTimeYYYYMMDD_HHMMSS());
    CxxUtilities::ROOTUtilities::writeObjString("detectorID", detectorID);
    if (configurationYAMLFile != "") {
      std::string configurationYAML = CxxUtilities::File::getAllLinesAsString(configurationYAMLFile);
      CxxUtilities::ROOTUtilities::writeObjString("configurationYAML", configurationYAML);
    }
  }
