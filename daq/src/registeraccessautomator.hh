#ifndef GROWTHDAQ_REGISTER_ACCESS_AUTOMATOR_HH_
#define GROWTHDAQ_REGISTER_ACCESS_AUTOMATOR_HH_

#include "spacewire/rmapinitiator.hh"
#include "spacewire/spacewireifoveruart.hh"

#include "growth-fpga/hitpatternmodule.hh"
#include "growth-fpga/slowadcdac.hh"

#include "yaml-cpp/yaml.h"
#include "spdlog/spdlog.h"

#include <sstream>
#include <memory>

class RegisterAccessAutomator {
 public:
  RegisterAccessAutomator(const std::string& deviceName) {
    spwif_ = std::make_unique<SpaceWireIFOverUART>(deviceName);
    const bool openResult = spwif_->open();
    assert(openResult);

    rmapEngine_ = std::make_shared<RMAPEngine>(spwif_.get());
    rmapEngine_->start();

    adcRMAPTargetNode_ = std::make_shared<RMAPTargetNode>();
    adcRMAPTargetNode_->setDefaultKey(0x00);

    slowAdcDacModule_ = std::make_unique<SlowAdcDac>(std::make_shared<RMAPInitiator>(rmapEngine_), adcRMAPTargetNode_);
    highvoltageControlGpioModule_ =
        std::make_unique<HighvoltageControlGpio>(std::make_shared<RMAPInitiator>(rmapEngine_), adcRMAPTargetNode_);
    hitPatternModule_ =
        std::make_unique<HitPatternModule>(std::make_shared<RMAPInitiator>(rmapEngine_), adcRMAPTargetNode_);
  }

  void run(const std::string& inputFileName) {
    YAML::Node yaml_root = YAML::LoadFile(inputFileName);
    assert(yaml_root.IsSequence());

    size_t entryIndex = 0;
    for (const auto& entry : yaml_root) {
      spdlog::info("Processing {}", entryIndex);
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
    } else if (module == "gpio") {
      if (command == "set") {
        executeHighvoltageControlSet(arguments);
      }
    } else {
      throw std::runtime_error("Invalid module name (" + module + ")");
    }
  }

  void executeHitPatternSet(const YAML::Node& arguments) {
    const auto ch = arguments["ch"].as<std::size_t>();
    const HitPatternSettings settings{arguments["lowerThreshold"].as<u16>(), arguments["upperThreshold"].as<u16>(),
                                      arguments["width"].as<u16>(), arguments["delay"].as<u16>()};
    spdlog::info("Setting hit-pattern registers (Ch.{}, lower = {}, upper = {}, width = {}, delay = {})", ch,
                 settings.lowerThreshold, settings.upperThreshold, settings.width, settings.delay);
    hitPatternModule_->writeSettings(ch, settings);
    spdlog::info("==> DONE");
  }

  void executeSlowDacSet(const YAML::Node& arguments) {
    const auto ch = arguments["ch"].as<std::size_t>();
    const u16 gain = arguments["ch"].as<u16>();
    const u16 outputValue = arguments["outputValue"].as<u16>();
    spdlog::info("Setting DAC (Ch.{}, gain = {}, outputValue = {})", ch, gain, outputValue);
    slowAdcDacModule_->setDac(ch, gain, outputValue);
    spdlog::info("==> DONE");
  }

  void executeHighvoltageControlSet(const YAML::Node& arguments) {
    const auto outputEnable = arguments["outputEnable"].as<bool>();
    spdlog::info("Setting HV output enable (enable = {})", (outputEnable ? "enabled" : "disabled"));
    highvoltageControlGpioModule_->outputEnable(outputEnable);
  }

  std::unique_ptr<SpaceWireIFOverUART> spwif_{};
  std::shared_ptr<RMAPEngine> rmapEngine_{};

  std::shared_ptr<RMAPTargetNode> adcRMAPTargetNode_{};
  std::unique_ptr<SlowAdcDac> slowAdcDacModule_{};
  std::unique_ptr<HighvoltageControlGpio> highvoltageControlGpioModule_{};
  std::unique_ptr<HitPatternModule> hitPatternModule_{};
};
#endif
