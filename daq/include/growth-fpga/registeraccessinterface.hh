#ifndef GROWTH_FPGA_REGISTER_ACCESS_INTERFACE_HH_
#define GROWTH_FPGA_REGISTER_ACCESS_INTERFACE_HH_

#include "types.h"
#include "spacewire/rmapinitiator.hh"

#include <memory>

class RegisterAccessInterface {
 public:
  RegisterAccessInterface(RMAPInitiatorPtr rmapInitiator, std::shared_ptr<RMAPTargetNode> rmapTargetNode);
  virtual ~RegisterAccessInterface() = default;
  u16 read16(u32 address) const;
  u32 read32(u32 address) const;
  u64 read48(u32 address) const;
  void read(u32 address, size_t numBytes, u8* buffer) const;
  void write(u32 address, u16 value);
  void write(u32 address, u32 value);

 private:
  void timeoutSleep();
  std::shared_ptr<RMAPInitiator> rmapInitiator_{};
  std::shared_ptr<RMAPTargetNode> rmapTargetNode_{};
  static constexpr size_t MAX_RETRIES = 5;
};

#endif
