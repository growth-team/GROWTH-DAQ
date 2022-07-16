#include "growth-fpga/semaphoreregister.hh"

#include <chrono>
#include <thread>

SemaphoreRegister::SemaphoreRegister(RMAPInitiatorPtr rmapInitiator, std::shared_ptr<RMAPTargetNode> rmapTargetNode,
                                     u32 address)
    : RegisterAccessInterface(std::move(rmapInitiator), std::move(rmapTargetNode)), address_(address) {}

void SemaphoreRegister::request() {
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

void SemaphoreRegister::release() {
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

void SemaphoreRegister::sleep() {
  // TODO: Implement timeout.
  using namespace std::chrono_literals;
  std::this_thread::sleep_for(10ms);
}

SemaphoreLock::SemaphoreLock(SemaphoreRegister& semaphore) : semaphore_(semaphore) { semaphore_.request(); }
SemaphoreLock::~SemaphoreLock() { semaphore_.release(); }
