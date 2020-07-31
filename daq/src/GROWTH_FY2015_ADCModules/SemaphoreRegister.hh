#ifndef SEMAPHOREREGISTER_HH_
#define SEMAPHOREREGISTER_HH_

#include "GROWTH_FY2015_ADCModules/RMAPHandlerUART.hh"
#include "GROWTH_FY2015_ADCModules/RegisterAccessInterface.hh"
#include "GROWTH_FY2015_ADCModules/Types.hh"
#include "types.h"

#include <chrono>
#include <memory>
#include <thread>

/// An internal class which represents a semaphore in the VHDL logic.
class SemaphoreRegister : public RegisterAccessInterface {
 public:
  /** Constructor.
   * @param rmaphandler instance of RMAPHandler which is used for this class to communicate the ADC board
   * @param address an address of which semaphore this instance should handle
   */
  SemaphoreRegister(std::shared_ptr<RMAPHandlerUART> rmapHandler, std::shared_ptr<RMAPTargetNode> rmapTargetNode,
                    u32 address)
      : RegisterAccessInterface(rmapHandler, rmapTargetNode), address_(address) {}

 public:
  /// Requests the semaphore.
  /// This method will wait for indefinitely until it successfully gets the semaphore.
  void request() {
    while (true) {
      constexpr u16 ACQUIRE = 0xFFFF;
      write(address_, ACQUIRE);
      semaphoreValue = read16(address_);
      if (semaphoreValue != 0x0000) {
        return;
      } else {
        sleep();
      }
    }
  }

  /// Releases the semaphore.
  void release() {
    while (true) {
      constexpr u16 RELEASE = 0x0000;
      write(address_, RELEASE);
      semaphoreValue = read16(address_);
      if (semaphoreValue == RELEASE) {
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
  u16 semaphoreValue = 0;
  std::mutex mutex_;
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
