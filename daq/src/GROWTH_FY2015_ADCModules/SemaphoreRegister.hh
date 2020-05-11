#ifndef SEMAPHOREREGISTER_HH_
#define SEMAPHOREREGISTER_HH_

#include "GROWTH_FY2015_ADCModules/RMAPHandler.hh"
#include "GROWTH_FY2015_ADCModules/Types.hh"

#include <chrono>
#include <memory>
#include <thread>

/// An internal class which represents a semaphore in the VHDL logic.
class SemaphoreRegister {
 public:
  /** Constructor.
   * @param rmaphandler instance of RMAPHandler which is used for this class to communicate the ADC board
   * @param address an address of which semaphore this instance should handle
   */
  SemaphoreRegister(std::shared_ptr<RMAPHandler> handler, uint32_t address)
      : rmaphandler_(handler), address_(address) {}

 public:
  /// Requests the semaphore.
  /// This method will wait for indefinitely until it successfully gets the semaphore.
  void request() {
    while (true) {
      rmaphandler_->setRegister(address_, 0xFFFF);
      semaphoreValue = rmaphandler_->getRegister(address_);
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
      rmaphandler_->setRegister(address_, 0x0000);
      semaphoreValue = rmaphandler_->getRegister(address_);
      if (semaphoreValue == 0x0000) {
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

  std::shared_ptr<RMAPHandler> rmaphandler_;
  uint32_t address_{};
  uint16_t semaphoreValue = 0;
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
