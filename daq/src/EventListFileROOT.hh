/*
 * EventListFileROOT.hh
 *
 *  Created on: Sep 29, 2015
 *      Author: yuasa
 */

#ifndef EVENTLISTFILEROOT_HH_
#define EVENTLISTFILEROOT_HH_

#include "EventListFile.hh"
#include "CxxUtilities/ROOTUtilities.hh"
#include "TTree.h"
#include "TFile.h"
#include "TH1D.h"
#include "TCanvas.h"
#include "TGraph.h"

class EventListFileROOT: public EventListFile {
private:
	TTree* eventTree;
	TFile* outputFile = NULL;
	std::string detectorID;
	SpaceFibreADC::Event eventEntry;
	std::string configurationYAMLFile;

public:
	EventListFileROOT(std::string fileName, std::string detectorID = "empty", std::string configurationYAMLFile = "") :
			EventListFile(fileName), detectorID(detectorID), configurationYAMLFile(configurationYAMLFile) {
		createOutputRootFile();
	}

public:
	~EventListFileROOT() {
		/*
		if (outputFile != NULL) {
			outputFile->Close();
		}*/
	}

private:
	void createOutputRootFile() {
		using namespace std;
		outputFile = new TFile(fileName.c_str(), "recreate");
		eventTree = new TTree("eventTree", "eventTree");

//    - C : a character string terminated by the 0 character
//     - B : an 8 bit signed integer (Char_t)
//     - b : an 8 bit unsigned integer (UChar_t)
//     - S : a 16 bit signed integer (Short_t)
//     - s : a 16 bit unsigned integer (UShort_t)
//     - I : a 32 bit signed integer (Int_t)
//     - i : a 32 bit unsigned integer (UInt_t)
//     - F : a 32 bit floating point (Float_t)
//     - D : a 64 bit floating point (Double_t)
//     - L : a 64 bit signed integer (Long64_t)
//     - l : a 64 bit unsigned integer (ULong64_t)
//     - O : [the letter 'o', not a zero] a boolean (Bool_t)

		eventTree->Branch("boardIndexAndChannel", &eventEntry.ch, "boardIndexAndChannel/i");
		//eventTree->Branch("unixTime", &eventEntry.unixTime, "unixTime/D");
		eventTree->Branch("timeTag", &eventEntry.timeTag, "timeTag/l");
		eventTree->Branch("triggerCount", &eventEntry.triggerCount, "triggerCount/i");
		eventTree->Branch("nSamples", &eventEntry.nSamples, "nSamples/i");
		//eventTree->Branch("energy", &eventEntry.energy, "energy/F");
		eventTree->Branch("phaMax", &eventEntry.phaMax, "phaMax/s");
		eventTree->Branch("waveform", eventEntry.waveform, "waveform[nSamples]/s");

		//write header info
		writeHeader();
	}

private:
	void writeHeader(){
		CxxUtilities::ROOTUtilities::writeObjString("fileCreationDate",
				CxxUtilities::Time::getCurrentTimeYYYYMMDD_HHMMSS());
		CxxUtilities::ROOTUtilities::writeObjString("detectorID", detectorID);
		if (configurationYAMLFile != "") {
			std::string configurationYAML = CxxUtilities::File::getAllLinesAsString(configurationYAMLFile);
			CxxUtilities::ROOTUtilities::writeObjString("configurationYAML", configurationYAML);
		}
	}

public:
	void fillEvents(std::vector<GROWTH_FY2015_ADC_Type::Event*>& events) {
		for (auto& event : events) {
			memcpy(&eventTree, &event, sizeof(GROWTH_FY2015_ADC_Type::Event));
			//eventTree->Fill();
		}
	}

public:
	size_t getEntries() {
		return eventTree->GetEntries();
	}

public:
	void fillGPSTime(uint8_t* gpsTimeRegisterBuffer) {}

public:
	void close() {
		//outputFile->Close();
		outputFile = NULL;
	}

};

#endif /* EVENTLISTFILEROOT_HH_ */
