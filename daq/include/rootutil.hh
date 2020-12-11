/*
 * ROOTUtilities.hh
 *
 *  Created on: Aug 29, 2015
 *      Author: yuasa
 */

#ifndef INCLUDES_CXXUTILITIES_ROOTUTILITIES_HH_
#define INCLUDES_CXXUTILITIES_ROOTUTILITIES_HH_

#include "CommonHeader.hh"
#include "TObjString.h"
#include "TString.h"
#include "TFile.h"
#include "TStyle.h"
#include "TAxis.h"
#include "TGraph.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TH1I.h"
#include "TH2I.h"
#include "TH1S.h"
#include "TH2S.h"
#include "TKey.h"

#include "TTimeStamp.h"

namespace CxxUtilities {
class ROOTUtilities {
	//============================================
	// Read TObjString from file
	//============================================
public:
	static double getObjStringAsDouble(TDirectory* dir, std::string name){
		auto str = (TObjString*)dir->Get(name.c_str());
		return CxxUtilities::String::toDouble(str->GetString());
	}
	static double readObjStringAsDouble(TDirectory* dir, std::string name){
		return getObjStringAsDouble(dir, name);
	}


public:
	static int getObjStringAsInteger(TDirectory* dir, std::string name){
		auto str = (TObjString*)dir->Get(name.c_str());
		return CxxUtilities::String::toInteger(str->GetString());
	}
	static int readObjStringAsInteger(TDirectory* dir, std::string name){
		return getObjStringAsInteger(dir, name);
	}

public:
	static bool getObjStringAsBoolean(TDirectory* dir, std::string name){
		auto str = (TObjString*)dir->Get(name.c_str());
		return CxxUtilities::String::toBoolean(std::string(str->GetString()));
	}
	static bool readObjStringAsBoolean(TDirectory* dir, std::string name){
		return getObjStringAsBoolean(dir, name);
	}

public:
	static std::string getObjStringAsString(TDirectory* dir, std::string name){
		auto str = (TObjString*)dir->Get(name.c_str());
		std::string s(str->GetString());
		return s;
	}
	static std::string readObjStringAsString(TDirectory* dir, std::string name){
		return getObjStringAsString(dir, name);
	}

	//============================================
	// Write TObjString to file
	//============================================
public:
	static void writeObjString(std::string name, std::string value) {
		auto str = new TObjString(value.c_str());
		str->Write(name.c_str());
	}

public:
	static void writeObjString(std::string name, double value) {
		auto str = new TObjString(Form("%f", value));
		str->Write(name.c_str());
	}

public:
	static void writeObjString(std::string name, float value) {
		auto str = new TObjString(Form("%f", value));
		str->Write(name.c_str());
	}

public:
	static void writeObjString(std::string name, size_t value) {
		auto str = new TObjString(Form("%d", value));
		str->Write(name.c_str());
	}

public:
	static void writeObjString(std::string name, uint32_t value) {
		auto str = new TObjString(Form("%d", value));
		str->Write(name.c_str());
	}

public:
	static void writeObjString(std::string name, uint16_t value) {
		auto str = new TObjString(Form("%d", value));
		str->Write(name.c_str());
	}

public:
	static void writeObjString(std::string name, uint8_t value) {
		auto str = new TObjString(Form("%d", value));
		str->Write(name.c_str());
	}

public:
	/** Copies TObjString instances from one TFile to another.
	 * @return the number of successfully copied TObjStrings
	 */
	static size_t copyObjString(TDirectory* from, TDirectory* to, std::vector<std::string> dirNames = { }) {
		using namespace std;
		size_t result = 0;

		//process top-level dir
		from->cd();
		cout << "copyObjString:" << from->GetListOfKeys()->GetEntries() << " at top-level" << endl;
		auto list = from->GetListOfKeys();
		for (size_t i = 0; i < list->GetEntries(); i++) {
			TKey* key = (TKey*) list->At(i);
			if (std::string(key->GetClassName()) == "TObjString") {
				to->cd();
				auto str = (TObjString*) (from->Get(key->GetName()));
				str->Write(key->GetName());
				result++;
			}
		}

		//process subdir
		for (auto dir : dirNames) {
			if (from->cd(dir.c_str())) {
				TDirectory* from_dir = gDirectory;
				cout << "copyObjString: " << from->GetListOfKeys()->GetEntries() << " in subdir " << dir << endl;
				auto list = from_dir->GetListOfKeys();
				for (size_t i = 0; i < list->GetEntries(); i++) {
					TKey* key = (TKey*) list->At(i);
					if (std::string(key->GetClassName()) == "TObjString") {
						to->cd();
						if (to->Get(dir.c_str()) == NULL) {
							to->mkdir(dir.c_str());
						}
						to->cd(dir.c_str());
						auto str = (TObjString*) (from_dir->Get(key->GetName()));
						str->Write(key->GetName());
						result++;
					}
				}
			}
		}
		return result;
	}

	//============================================
	// Plot-style related
	//============================================
public:
	static void setFont(TGraph* obj, Font_t font = 63, int size = 13) {
		setFont( { obj->GetXaxis(), obj->GetYaxis() }, font, size);
	}

public:
	static void setFont(TH1* obj, Font_t font = 63, int size = 13) {
		setFont( { obj->GetXaxis(), obj->GetYaxis() }, font, size);
	}

private:
	static void setFont(std::vector<TAxis*> axes, Font_t font, int size) {
		for (auto axis : axes) {
			axis->SetTitleFont(font);
			axis->SetLabelFont(font);
			axis->SetTitleSize(size);
			axis->SetLabelSize(size);
			if (font == 63 || font == 62) {
				axis->SetTitleOffset(2);
				gStyle->SetTitleFont(font, "t");
				gStyle->SetTitleSize(size);
			}
		}
	}
};

} //end of namespace CxxUtilities

#endif /* INCLUDES_CXXUTILITIES_ROOTUTILITIES_HH_ */
