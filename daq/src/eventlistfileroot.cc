#include "eventlistfileroot.hh"

EventListFileROOT::EventListFileROOT(std::string fileName, std::string detectorID = "empty",
                                     std::string configurationYAMLFile = "")
    : EventListFile(fileName), detectorID_(detectorID), configurationYAMLFile_(configurationYAMLFile) {
  eventEntry_.waveform = new u16[GROWTH_FY2015_ADC_Type::MaxWaveformLength];
  createOutputRootFile();
}

EventListFileROOT::~EventListFileROOT() {
  close();
  delete[] eventEntry_.waveform;
}

void EventListFileROOT::fillEvents(const std::vector<GROWTH_FY2015_ADC_Type::Event*>& events) {
  unixTime_ = timeutil::getUNIXTimeAsUInt64();
  for (auto& event : events) {
    copyEventData(event, &eventEntry_);
    eventTree_->Fill();
  }
}

size_t EventListFileROOT::getEntries() const { return eventTree_->GetEntries(); }

void EventListFileROOT::fillGPSTime(const u8* gpsTimeRegisterBuffer) {
  // "GPS time vs local clock" table is not implemented in the ROOT file format.
}

void EventListFileROOT::close() {
  if (outputFile_ != NULL) {
    eventTree_->Write();
    outputFile_->Close();
  }
}

void EventListFileROOT::createOutputRootFile() {
  outputFile_.reset(new TFile(fileName_.c_str(), "recreate"));
  eventTree_.reset(new TTree("eventTree", "eventTree"));

  // - C : a character string terminated by the 0 character
  // - B : an 8 bit signed integer (Char_t)
  // - b : an 8 bit unsigned integer (UChar_t)
  // - S : a 16 bit signed integer (Short_t)
  // - s : a 16 bit unsigned integer (UShort_t)
  // - I : a 32 bit signed integer (Int_t)
  // - i : a 32 bit unsigned integer (UInt_t)
  // - F : a 32 bit f32ing point (Float_t)
  // - D : a 64 bit f32ing point (Double_t)
  // - L : a 64 bit signed integer (Long64_t)
  // - l : a 64 bit unsigned integer (ULong64_t)
  // - O : [the letter 'o', not a zero] a boolean (Bool_t)

  eventTree_->Branch("boardIndexAndChannel", &eventEntry_.ch, "boardIndexAndChannel/b");
  eventTree_->Branch("timeTag", &eventEntry_.timeTag, "timeTag/l");
  eventTree_->Branch("unixTime", &unixTime_, "unixTime/i");
  eventTree_->Branch("triggerCount", &eventEntry_.triggerCount, "triggerCount/s");
  eventTree_->Branch("nSamples", &eventEntry_.nSamples, "nSamples/s");
  eventTree_->Branch("phaMax", &eventEntry_.phaMax, "phaMax/s");
  eventTree_->Branch("phaMaxTime", &eventEntry_.phaMax, "phaMaxTime/s");
  eventTree_->Branch("phaMin", &eventEntry_.phaMax, "phaMin/s");
  eventTree_->Branch("phaFirst", &eventEntry_.phaMax, "phaFirst/s");
  eventTree_->Branch("phaLast", &eventEntry_.phaMax, "phaLast/s");
  eventTree_->Branch("maxDerivative", &eventEntry_.phaMax, "maxDerivative/s");
  eventTree_->Branch("baseline", &eventEntry_.phaMax, "baseline/s");
  eventTree_->Branch("waveform", eventEntry_.waveform, "waveform[nSamples]/s");

  writeHeader();
}

void EventListFileROOT::copyEventData(GROWTH_FY2015_ADC_Type::Event* from, GROWTH_FY2015_ADC_Type::Event* to) {
  to->ch = from->ch;
  to->timeTag = from->timeTag;
  to->triggerCount = from->triggerCount;
  to->phaMax = from->phaMax;
  to->phaMaxTime = from->phaMaxTime;
  to->phaMin = from->phaMin;
  to->phaFirst = from->phaFirst;
  to->phaLast = from->phaLast;
  to->maxDerivative = from->maxDerivative;
  to->baseline = from->baseline;
  to->nSamples = from->nSamples;
  memcpy(to->waveform, from->waveform, sizeof(u16) * from->nSamples);
}

void EventListFileROOT::writeHeader() {
  CxxUtilities::ROOTUtilities::writeObjString("fileCreationDate", timeutil::getCurrentTimeYYYYMMDD_HHMMSS());
  CxxUtilities::ROOTUtilities::writeObjString("detectorID", detectorID_);
  if (configurationYAMLFile_ != "") {
    std::ifstream ifs(configurationYAMLFile_);
    const std::string configurationYAML(std::istreambuf_iterator<char>{ifs}, {});
    CxxUtilities::ROOTUtilities::writeObjString("configurationYAML", configurationYAML);
  }
}
