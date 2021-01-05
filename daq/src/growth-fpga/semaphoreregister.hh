#ifndef GROWTH_FPGA_SEMAPHOREREGISTER_HH_
#define GROWTH_FPGA_SEMAPHOREREGISTER_HH_

#include "types.h"

#include "growth-fpga/registeraccessinterface.hh"
#include "growth-fpga/types.hh"

#include <chrono>
#include <memory>
#include <mutex>
#include <thread>

/// An internal class which represents a semaphore in the VHDL logic.
class SemaphoreRegister : public RegisterAccessInterface {
 public:
  SemaphoreRegister(RMAPInitiatorPtr rmapInitiator, std::shared_ptr<RMAPTargetNode> rmapTargetNode, u32 address)
      : RegisterAccessInterface(std::move(rmapInitiator), std::move(rmapTargetNode)), address_(address) {}

 public:
  /// This method will wait for indefinitely until it successfully gets the semaphore.
  void request() {
    while (true) {
      constexpr u16 ACQUIRE = 0xFFFF;
      write(address_, ACQUIRE);
      semaphoreValue_ = read16(address_);
      if (semaphoreValue_ != 0x0000) {
        return;
      } else {
        sleep();
      }
    }
  }

  void release() {
    while (true) {
      constexpr u16 RELEASE = 0x0000;
      write(address_, RELEASE);
      semaphoreValue_ = read16(address_);
      if (semaphoreValue_ == RELEASE) {
        return;
      } else {
        sleep();
      }
    }
  }

 private:
  void sleep() {
    // TODO: Implement timeout.
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(10ms);
  }

  u32 address_{};
  u16 semaphoreValue_ = 0;
  std::mutex mutex_{};
};

/// RAII lock using SemaphoreRegister.
class SemaphoreLock {
 public:
  SemaphoreLock(SemaphoreRegister& semaphore) : semaphore_(semaphore) { semaphore_.request(); }
  ~SemaphoreLock() { semaphore_.release(); }

 private:
  SemaphoreRegister& semaphore_;
};
#endif /* SEMAPHOREREGISTER_HH_ */
