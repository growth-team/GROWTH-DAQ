#ifndef GROWTH_FPGA_SEMAPHOREREGISTER_HH_
#define GROWTH_FPGA_SEMAPHOREREGISTER_HH_

#include "types.h"

#include "growth-fpga/registeraccessinterface.hh"
#include "growth-fpga/types.hh"

#include <memory>
#include <mutex>

/// An internal class which represents a semaphore in the VHDL logic.
class SemaphoreRegister : public RegisterAccessInterface {
 public:
  SemaphoreRegister(RMAPInitiatorPtr rmapInitiator, std::shared_ptr<RMAPTargetNode> rmapTargetNode, u32 address);

 public:
  /// This method will wait for indefinitely until it successfully gets the semaphore.
  void request();
  void release();

 private:
  void sleep();

  u32 address_{};
  u16 semaphoreValue_ = 0;
  std::mutex mutex_{};
};

/// RAII lock using SemaphoreRegister.
class SemaphoreLock {
 public:
  explicit SemaphoreLock(SemaphoreRegister& semaphore);
  ~SemaphoreLock();

 private:
  SemaphoreRegister& semaphore_;
};
#endif /* SEMAPHOREREGISTER_HH_ */
