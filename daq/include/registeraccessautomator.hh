#ifndef GROWTHDAQ_REGISTER_ACCESS_AUTOMATOR_HH_
#define GROWTHDAQ_REGISTER_ACCESS_AUTOMATOR_HH_

#include "spacewire/rmapinitiator.hh"
#include "spacewire/spacewireifoveruart.hh"

#include "growth-fpga/hitpatternmodule.hh"
#include "growth-fpga/slowadcdac.hh"

#include "yaml-cpp/yaml.h"

#include <memory>

class RegisterAccessAutomator {
 public:
  RegisterAccessAutomator(const std::string& deviceName);
  void run(const std::string& inputFileName);

 private:
  void checkMustHaveKeys(const YAML::Node& entry) const;
  void executeCommand(const YAML::Node& entry);
  void executeHitPatternSet(const YAML::Node& arguments);
  void executeHitPatternOrSwitch(const YAML::Node& arguments);
  void executeSlowDacSet(const YAML::Node& arguments);
  void executeHighvoltageControlSet(const YAML::Node& arguments);

  std::unique_ptr<SpaceWireIFOverUART> spwif_{};
  std::shared_ptr<RMAPEngine> rmapEngine_{};

  std::shared_ptr<RMAPTargetNode> adcRMAPTargetNode_{};
  std::unique_ptr<SlowAdcDac> slowAdcDacModule_{};
  std::unique_ptr<HighvoltageControlGpio> highvoltageControlGpioModule_{};
  std::unique_ptr<HitPatternModule> hitPatternModule_{};
};

#endif
