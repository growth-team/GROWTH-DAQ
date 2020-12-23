#ifndef GROWTHDAQ_REGISTER_ACCESS_AUTOMATOR_HH_
#define GROWTHDAQ_REGISTER_ACCESS_AUTOMATOR_HH_


#include "growth-fpga/rmaphandleruart.hh"
#include "growth-fpga/hitpatternmodule.hh"
#include "growth-fpga/slowadcdac.hh"

#include "yaml-cpp/yaml.h"

#include <sstream>
#include <memory>

class RegisterAccessAutomator {
 public:
  RegisterAccessAutomator(const std::string& deviceName) : rmapHandler_(std::make_shared<RMAPHandlerUART>(deviceName)) {
    adcRMAPTargetNode_ = std::make_shared<RMAPTargetNode>();
    adcRMAPTargetNode_->setDefaultKey(0x00);
    adcRMAPTargetNode_->setReplyAddress({});
    adcRMAPTargetNode_->setTargetSpaceWireAddress({});
    adcRMAPTargetNode_->setTargetLogicalAddress(0xFE);
    adcRMAPTargetNode_->setInitiatorLogicalAddress(0xFE);

    slowAdcDacModule_ = std::make_unique<SlowAdcDac>(rmapHandler_, adcRMAPTargetNode_);
    highvoltageControlGpioModule_ = std::make_unique<HighvoltageControlGpio>(rmapHandler_, adcRMAPTargetNode_);
    hitPatternModule_ = std::make_unique<HitPatternModule>(rmapHandler_, adcRMAPTargetNode_);
  }
  void run(const std::string& inputFileName) {
    YAML::Node yaml_root = YAML::LoadFile(inputFileName);
    assert(yaml_root.IsSequence());

    size_t entryIndex = 0;
    for (const auto& entry : yaml_root) {
      // TODO: use proper logging library
      std::cout << "Processing " << entryIndex << std::endl;
      checkMustHaveKeys(entry);
      executeCommand(entry);
      entryIndex++;
    }
  }

 private:
  void checkMustHaveKeys(const YAML::Node& entry) {
    const std::vector<std::string> mustExistKeywords = {"command", "module", "arguments"};
    assert(entry.IsMap());
    for (const auto keyword : mustExistKeywords) {
      if (!entry[keyword].IsDefined()) {
        std::stringstream ss;
        ss << keyword << " is not defined in the configuration file";
        throw std::runtime_error(ss.str());
      }
    }
    assert(entry["arguments"].IsMap());
  }

  void executeCommand(const YAML::Node& entry) {
    const auto module = entry["module"].as<std::string>();
    const auto command = entry["command"].as<std::string>();
    const auto arguments = entry["arguments"];
    if (module == "hitpattern") {
      if (command == "set") {
        executeHitPatternSet(arguments);
      }
    } else if (module == "slowdac") {
      if (command == "set") {
        executeSlowDacSet(arguments);
      }
    } else {
      throw std::runtime_error("Invalid module name (" + module + ")");
    }
  }

  void executeHitPatternSet(const YAML::Node& arguments) {
    const auto ch = arguments["ch"].as<std::size_t>();
    const HitPatternSettings settings{arguments["lowerThreshold"].as<u16>(), arguments["upperThreshold"].as<u16>(),
                                      arguments["width"].as<u16>(), arguments["delay"].as<u16>()};
    std::cout << "Setting hit-pattern registers (Ch." << ch << ", lower = " << settings.lowerThreshold
              << ", upper = " << settings.upperThreshold << ", width = " << settings.width
              << ", delay = " << settings.delay << ")" << std::endl;
    hitPatternModule_->writeSettings(ch, settings);
    std::cout << MSG_DONE << std::endl;
  }

  void executeSlowDacSet(const YAML::Node& arguments) {
    const auto ch = arguments["ch"].as<std::size_t>();
    const u16 gain = arguments["ch"].as<u16>();
    const u16 outputValue = arguments["outputValue"].as<u16>();
    std::cout << "Setting DAC (Ch." << ch << ", gain = " << gain << ", outputValue = " << outputValue << ")"
              << std::endl;
    slowAdcDacModule_->setDac(ch, gain, outputValue);
    std::cout << MSG_DONE << std::endl;
  }

  void executeHighvoltageControlSet(const YAML::Node& arguments) {
    const auto outputEnable = arguments["outputEnable"].as<bool>();
    std::cout << "Setting HV output enable (enable = " << (outputEnable ? "enabled" : "disabled") << ")" << std::endl;
    highvoltageControlGpioModule_->outputEnable(outputEnable);
  }

  std::shared_ptr<RMAPHandlerUART> rmapHandler_;
  std::shared_ptr<RMAPTargetNode> adcRMAPTargetNode_;
  std::unique_ptr<SlowAdcDac> slowAdcDacModule_;
  std::unique_ptr<HighvoltageControlGpio> highvoltageControlGpioModule_;
  std::unique_ptr<HitPatternModule> hitPatternModule_;

  static constexpr std::string_view MSG_DONE = "  ==> DONE";
};
#endif