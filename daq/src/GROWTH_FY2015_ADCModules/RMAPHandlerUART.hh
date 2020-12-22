#ifndef RMAPHandlerUART_HH_
#define RMAPHandlerUART_HH_

#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>

#include "spacewire/rmapinitiator.hh"
#include "spacewire/spacewireifoveruart.hh"
#include "types.h"

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

  void read(const RMAPTargetNode* rmapTargetNode, u32 memoryAddress, u32 length, u8* buffer) {
    assert(!rmapInitiator_);
    for (size_t i = 0; i < maxNTrials; i++) {
      try {
        rmapInitiator_->read(rmapTargetNode, memoryAddress, length, buffer, timeOutDuration);
        break;
      } catch (RMAPInitiatorException& e) {
        std::cerr << "RMAPHandler::read() 1: RMAPInitiatorException::" << e.toString() << std::endl;
        std::cerr << "Read timed out (address="
                  << "0x" << std::hex << std::right << std::setw(8) << std::setfill('0') << memoryAddress
                  << " length=" << std::dec << length << "); trying again..." << std::endl;
        spwif_->cancelReceive();
        if (i == maxNTrials - 1) {
          assert(false && "Transaction failed");
        }
        usleep(100);
      }
    }
  }

  void write(const RMAPTargetNode* rmapTargetNode, u32 memoryAddress, u32 length, const u8* data) {
    assert(!rmapInitiator_);
    for (size_t i = 0; i < maxNTrials; i++) {
      try {
        if (length != 0) {
          rmapInitiator_->write(rmapTargetNode, memoryAddress, const_cast<u8*>(data), length, timeOutDuration);
        } else {
          rmapInitiator_->write(rmapTargetNode, memoryAddress, nullptr, 0, timeOutDuration);
        }
        break;
      } catch (RMAPInitiatorException& e) {
        std::cerr << "RMAPHandler::write() RMAPInitiatorException::" << e.toString() << std::endl;
        std::cerr << "Time out; trying again..." << std::endl;
        spwif_->cancelReceive();
        if (i == maxNTrials - 1) {
          assert(false && "Transaction failed");
        }
        usleep(100);
      }
    }
  }

  std::shared_ptr<RMAPEngine> getRMAPEngine() const { return rmapEngine_; }

 private:
  std::string deviceName_{};
  std::shared_ptr<SpaceWireIFOverUART> spwif_{};
  std::shared_ptr<RMAPEngine> rmapEngine_{};
  std::shared_ptr<RMAPInitiator> rmapInitiator_{};
  f64 timeOutDuration = 1000.0;
  size_t maxNTrials = 5;
};

#endif
