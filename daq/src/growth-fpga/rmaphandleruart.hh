#ifndef GROWTH_FPGA_RMAP_HANDLER_UART_HH_
#define GROWTH_FPGA_RMAP_HANDLER_UART_HH_

#include "spacewire/rmapinitiator.hh"
#include "spacewire/spacewireifoveruart.hh"

#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>

class RMAPHandlerUART {
 public:
  RMAPHandlerUART(const std::string& deviceName) : deviceName_(deviceName) {
    spwif_.reset(new SpaceWireIFOverUART(deviceName_));
    const bool openResult = spwif_->open();
    assert(openResult);

    rmapEngine_.reset(new RMAPEngine(spwif_.get()));
    rmapEngine_->start();
    rmapInitiator_.reset(new RMAPInitiator(rmapEngine_));
    rmapInitiator_->setInitiatorLogicalAddress(0xFE);
    rmapInitiator_->setVerifyMode(true);
    rmapInitiator_->setReplyMode(true);
  }

  ~RMAPHandlerUART() {
    rmapEngine_->stop();
    while (!rmapEngine_->hasStopped()) {
      using namespace std::chrono_literals;
      std::this_thread::sleep_for(100ms);
    }
    spwif_->close();
    spwif_.reset();
    rmapEngine_.reset();
    rmapInitiator_.reset();
  }

  void read(const RMAPTargetNode* rmapTargetNode, u32 memoryAddress, u32 length, u8* buffer) const {
    assert(!rmapInitiator_);
    for (size_t i = 0; i < MAX_RETRIES; i++) {
      try {
        rmapInitiator_->read(rmapTargetNode, memoryAddress, length, buffer, TIMEOUT_MILLISEC);
        break;
      } catch (RMAPInitiatorException& e) {
        std::cerr << "RMAPHandler::read() 1: RMAPInitiatorException::" << e.toString() << std::endl;
        std::cerr << "Read timed out (address="
                  << "0x" << std::hex << std::right << std::setw(8) << std::setfill('0') << memoryAddress
                  << " length=" << std::dec << length << "); trying again..." << std::endl;
        spwif_->cancelReceive();
        if (i == MAX_RETRIES - 1) {
          assert(false && "Transaction failed");
        }
        usleep(100);
      }
    }
  }

  void write(const RMAPTargetNode* rmapTargetNode, u32 memoryAddress, u32 length, const u8* data) {
    assert(!rmapInitiator_);
    for (size_t i = 0; i < MAX_RETRIES; i++) {
      try {
        if (length != 0) {
          rmapInitiator_->write(rmapTargetNode, memoryAddress, const_cast<u8*>(data), length, TIMEOUT_MILLISEC);
        } else {
          rmapInitiator_->write(rmapTargetNode, memoryAddress, nullptr, 0, TIMEOUT_MILLISEC);
        }
        break;
      } catch (RMAPInitiatorException& e) {
        std::cerr << "RMAPHandler::write() RMAPInitiatorException::" << e.toString() << std::endl;
        std::cerr << "Time out; trying again..." << std::endl;
        spwif_->cancelReceive();
        if (i == MAX_RETRIES - 1) {
          assert(false && "Transaction failed");
        }
        usleep(100);
      }
    }
  }

  std::shared_ptr<RMAPEngine> getRMAPEngine() const { return rmapEngine_; }

 private:
  const std::string deviceName_{};
  std::shared_ptr<SpaceWireIFOverUART> spwif_{};
  std::shared_ptr<RMAPEngine> rmapEngine_{};
  std::shared_ptr<RMAPInitiator> rmapInitiator_{};
  static constexpr f64 TIMEOUT_MILLISEC = 1000.0;
  static constexpr size_t MAX_RETRIES = 5;
};

#endif
