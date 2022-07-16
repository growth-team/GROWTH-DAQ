#ifndef GROWTHDAQ_ROOTUTIL_HH_
#define GROWTHDAQ_ROOTUTIL_HH_

#include "TAxis.h"
#include "TFile.h"
#include "TGraph.h"
#include "TH1D.h"
#include "TH1F.h"
#include "TH1I.h"
#include "TH1S.h"
#include "TH2D.h"
#include "TH2F.h"
#include "TH2I.h"
#include "TH2S.h"
#include "TKey.h"
#include "TObjString.h"
#include "TString.h"
#include "TStyle.h"
#include "TTimeStamp.h"

#include <string>
#include "stringutil.hh"

namespace rootutil {
f64 getObjStringAsDouble(TDirectory* dir, std::string name) {
  auto str = (TObjString*)dir->Get(name.c_str());
  return stringutil::toDouble(str->GetString());
}
f64 readObjStringAsDouble(TDirectory* dir, std::string name) { return getObjStringAsDouble(dir, name); }

i32 getObjStringAsInteger(TDirectory* dir, std::string name) {
  auto str = (TObjString*)dir->Get(name.c_str());
  return stringutil::toInteger(str->GetString());
}
i32 readObjStringAsInteger(TDirectory* dir, std::string name) { return getObjStringAsInteger(dir, name); }

bool getObjStringAsBoolean(TDirectory* dir, std::string name) {
  auto str = (TObjString*)dir->Get(name.c_str());
  return stringutil::toBoolean(std::string(str->GetString()));
}
bool readObjStringAsBoolean(TDirectory* dir, std::string name) { return getObjStringAsBoolean(dir, name); }

std::string getObjStringAsString(TDirectory* dir, std::string name) {
  auto str = (TObjString*)dir->Get(name.c_str());
  return str->GetString();
}
std::string readObjStringAsString(TDirectory* dir, std::string name) { return getObjStringAsString(dir, name); }

//============================================
// Write TObjString to file
//============================================

void writeObjString(std::string name, std::string value) {
  auto str = new TObjString(value.c_str());
  str->Write(name.c_str());
}

void writeObjString(std::string name, f64 value) {
  auto str = new TObjString(Form("%f", value));
  str->Write(name.c_str());
}

void writeObjString(std::string name, float value) {
  auto str = new TObjString(Form("%f", value));
  str->Write(name.c_str());
}

void writeObjString(std::string name, size_t value) {
  auto str = new TObjString(Form("%d", value));
  str->Write(name.c_str());
}

void writeObjString(std::string name, uint32_t value) {
  auto str = new TObjString(Form("%d", value));
  str->Write(name.c_str());
}

void writeObjString(std::string name, uint16_t value) {
  auto str = new TObjString(Form("%d", value));
  str->Write(name.c_str());
}

void writeObjString(std::string name, uint8_t value) {
  auto str = new TObjString(Form("%d", value));
  str->Write(name.c_str());
}

/** Copies TObjString instances from one TFile to another.
 * @return the number of successfully copied TObjStrings
 */
size_t copyObjString(TDirectory* from, TDirectory* to, std::vector<std::string> dirNames = {}) {
  using namespace std;
  size_t result = 0;

  // process top-level dir
  from->cd();
  cout << "copyObjString:" << from->GetListOfKeys()->GetEntries() << " at top-level" << endl;
  auto list = from->GetListOfKeys();
  for (size_t i = 0; i < list->GetEntries(); i++) {
    TKey* key = (TKey*)list->At(i);
    if (std::string(key->GetClassName()) == "TObjString") {
      to->cd();
      auto str = (TObjString*)(from->Get(key->GetName()));
      str->Write(key->GetName());
      result++;
    }
  }

  // process subdir
  for (auto dir : dirNames) {
    if (from->cd(dir.c_str())) {
      TDirectory* from_dir = gDirectory;
      cout << "copyObjString: " << from->GetListOfKeys()->GetEntries() << " in subdir " << dir << endl;
      auto list = from_dir->GetListOfKeys();
      for (size_t i = 0; i < list->GetEntries(); i++) {
        TKey* key = (TKey*)list->At(i);
        if (std::string(key->GetClassName()) == "TObjString") {
          to->cd();
          if (to->Get(dir.c_str()) == NULL) {
            to->mkdir(dir.c_str());
          }
          to->cd(dir.c_str());
          auto str = (TObjString*)(from_dir->Get(key->GetName()));
          str->Write(key->GetName());
          result++;
        }
      }
    }
  }
  return result;
}

};  // namespace rootutil
}

#endif
