#ifndef RMAPHandlerUART_HH_
#define RMAPHandlerUART_HH_

#include "GROWTH_FY2015_ADCModules/RMAPHandler.hh"
#include "SpaceWireIFOverUART.hh"

#include "CxxUtilities/CxxUtilities.hh"

#include <fstream>
#include <iostream>
#include <vector>

class RMAPHandlerUART : public RMAPHandler {
 public:
  RMAPHandlerUART(std::string deviceName, std::vector<RMAPTargetNode*> rmapTargetNodes)
      : RMAPHandler(), deviceName_(deviceName), timeOutDuration(2000.0), maxNTrials(10), useDraftECRC(false) {
    this->setTimeOutDuration(DefaultTimeOut);

    // add RMAPTargetNodes to DB
    for (auto& rmapTargetNode : rmapTargetNodes) {
      rmapTargetDB.addRMAPTargetNode(rmapTargetNode);
    }

    adcRMAPTargetNode = rmapTargetDB.getRMAPTargetNode("ADCBox");
    if (adcRMAPTargetNode == NULL) {
      cerr << "RMAPHandlerUART::RMAPHandlerUART(): ADCBox RMAP Target Node not found" << endl;
      exit(-1);
    }
  }
  virtual ~RMAPHandlerUART() {}

 public:
  bool connectoToSpaceWireToGigabitEther() {
    using namespace std;
    if (spwif_ != NULL) {
      delete spwif_;
    }

    // connect to UART-USB-SpaceWire interface
    spwif_ = new SpaceWireIFOverUART(this->deviceName);
    try {
      spwif->open();
      _isConnectedToSpWGbE = true;
    } catch (...) {
      cout << "RMAPHandlerUART::connectoToSpaceWireToGigabitEther() connection failed (" << this->deviceName << ")"
           << endl;
      _isConnectedToSpWGbE = false;
      return false;
    }

    // start RMAPEngine
    rmapEngine = new RMAPEngine(spwif_);
    if (useDraftECRC) {
      cout << "RMAPHandlerUART::connectoToSpaceWireToGigabitEther() setting DraftE CRC mode to RMAPEngine" << endl;
      rmapEngine->setUseDraftECRC(true);
    }
    rmapEngine->start();
    rmapInitiator = new RMAPInitiator(rmapEngine);
    rmapInitiator->setInitiatorLogicalAddress(0xFE);
    rmapInitiator->setVerifyMode(true);
    rmapInitiator->setReplyMode(true);
    if (useDraftECRC) {
      rmapInitiator->setUseDraftECRC(true);
      cout << "RMAPHandlerUART::connectoToSpaceWireToGigabitEther() setting DraftE CRC mode to RMAPInitiator" << endl;
    }

    return true;
  }

 public:
  virtual void disconnectSpWGbE() {
    _isConnectedToSpWGbE = false;

    using namespace std;
    cout << "RMAPHandler::disconnectSpWGbE(): Stopping RMAPEngine" << endl;
    rmapEngine->stop();
    while (!rmapEngine->hasStopped) {
      CxxUtilities::Condition c;
      c.wait(100);
      cout << "Waiting for RMAPEngine to completely stop." << endl;
    }
    cout << "RMAPHandler::disconnectSpWGbE(): Closing SpaceWire interface" << endl;
    this->spwif->close();
    cout << "RMAPHandler::disconnectSpWGbE(): Deleting instances" << endl;
    delete rmapEngine;
    delete rmapInitiator;
    delete spwif;

    spwif_ = NULL;
    rmapEngine = NULL;
    rmapInitiator = NULL;
    cout << "RMAPHandler::disconnectSpWGbE(): Completed" << endl;
  }

 public:
  void read(std::string rmapTargetNodeID, uint32_t memoryAddress, uint32_t length, uint8_t* buffer) {
    RMAPTargetNode* targetNode;
    try {
      targetNode = rmapTargetDB.getRMAPTargetNode(rmapTargetNodeID);
    } catch (...) {
      throw RMAPHandlerException(RMAPHandlerException::NoSuchTarget);
    }
    read(targetNode, memoryAddress, length, buffer);
  }

 public:
  void read(std::string rmapTargetNodeID, std::string memoryObjectID, uint8_t* buffer) {
    RMAPTargetNode* targetNode;
    try {
      targetNode = rmapTargetDB.getRMAPTargetNode(rmapTargetNodeID);
    } catch (...) {
      std::cout << "NoSuchTarget" << std::endl;
      throw RMAPHandlerException(RMAPHandlerException::NoSuchTarget);
    }
    read(targetNode, memoryObjectID, buffer);
  }

 public:
  void read(RMAPTargetNode* rmapTargetNode, uint32_t memoryAddress, uint32_t length, uint8_t* buffer) {
    using namespace std;
    if (rmapInitiator == NULL) {
      return;
    }
    for (size_t i = 0; i < maxNTrials; i++) {
      try {
        rmapInitiator->read(rmapTargetNode, memoryAddress, length, buffer, timeOutDuration);
        break;
      } catch (RMAPInitiatorException& e) {
        cerr << "RMAPHandler::read() 1: RMAPInitiatorException::" << e.toString() << endl;
        std::cerr << "Read timed out (address="
                  << "0x" << hex << right << setw(8) << setfill('0') << memoryAddress << " length=" << dec << length
                  << "); trying again..." << std::endl;
        spwif_->cancelReceive();
        if (i == maxNTrials - 1) {
          if (e.getStatus() == RMAPInitiatorException::Timeout) {
            throw RMAPHandlerException(RMAPHandlerException::TimeOut);
          } else {
            throw RMAPHandlerException(RMAPHandlerException::LowerException);
          }
        }
        usleep(100);
      }
    }
  }

 public:
  void read(RMAPTargetNode* rmapTargetNode, std::string memoryObjectID, uint8_t* buffer) {
    using namespace std;
    if (rmapInitiator == NULL) {
      return;
    }
    for (size_t i = 0; i < maxNTrials; i++) {
      try {
        rmapInitiator->read(rmapTargetNode, memoryObjectID, buffer, timeOutDuration);
        break;
      } catch (RMAPInitiatorException& e) {
        cerr << "RMAPHandler::read() 2: RMAPInitiatorException::" << e.toString() << endl;
        spwif_->cancelReceive();
        if (i == maxNTrials - 1) {
          if (e.getStatus() == RMAPInitiatorException::Timeout) {
            throw RMAPHandlerException(RMAPHandlerException::TimeOut);
          } else {
            throw RMAPHandlerException(RMAPHandlerException::LowerException);
          }
        }
        usleep(100);
      } catch (RMAPReplyException& e) {
        std::cerr << "RMAPReplyException" << std::endl;
        throw RMAPHandlerException(RMAPHandlerException::LowerException);
      }
    }
  }

 public:
  void write(std::string rmapTargetNodeID, uint32_t memoryAddress, uint8_t* data, uint32_t length) {
    RMAPTargetNode* targetNode;
    try {
      targetNode = rmapTargetDB.getRMAPTargetNode(rmapTargetNodeID);
    } catch (...) {
      throw RMAPHandlerException(RMAPHandlerException::NoSuchTarget);
    }
    write(targetNode, memoryAddress, data, length);
  }

 public:
  void write(std::string rmapTargetNodeID, std::string memoryObjectID, uint8_t* data) {
    RMAPTargetNode* targetNode;
    try {
      targetNode = rmapTargetDB.getRMAPTargetNode(rmapTargetNodeID);
    } catch (...) {
      throw RMAPHandlerException(RMAPHandlerException::NoSuchTarget);
    }
    write(targetNode, memoryObjectID, data);
  }

 public:
  void write(RMAPTargetNode* rmapTargetNode, uint32_t memoryAddress, uint8_t* data, uint32_t length) {
    using namespace std;
    if (rmapInitiator == NULL) {
      return;
    }
    for (size_t i = 0; i < maxNTrials; i++) {
      try {
        if (length != 0) {
          rmapInitiator->write(rmapTargetNode, memoryAddress, data, length, timeOutDuration);
        } else {
          rmapInitiator->write(rmapTargetNode, memoryAddress, nullptr, 0, timeOutDuration);
        }
        break;
      } catch (RMAPInitiatorException& e) {
        cerr << "RMAPHandler::write() 1: RMAPInitiatorException::" << e.toString() << endl;
        cerr << "Time out; trying again..." << endl;
        spwif->cancelReceive();
        if (i == maxNTrials - 1) {
          if (e.getStatus() == RMAPInitiatorException::Timeout) {
            throw RMAPHandlerException(RMAPHandlerException::TimeOut);
          } else {
            throw RMAPHandlerException(RMAPHandlerException::LowerException);
          }
        }
        usleep(100);
      }
    }
  }

 public:
  void write(RMAPTargetNode* rmapTargetNode, std::string memoryObjectID, uint8_t* data) {
    using namespace std;
    if (rmapInitiator == NULL) {
      return;
    }
    for (size_t i = 0; i < maxNTrials; i++) {
      try {
        rmapInitiator->write(rmapTargetNode, memoryObjectID, data, timeOutDuration);
      } catch (RMAPInitiatorException& e) {
        cerr << "RMAPHandler::write() 2: RMAPInitiatorException::" << e.toString() << endl;
        cerr << "Time out; trying again..." << endl;
        spwif->cancelReceive();
        if (i == maxNTrials - 1) {
          if (e.getStatus() == RMAPInitiatorException::Timeout) {
            throw RMAPHandlerException(RMAPHandlerException::TimeOut);
          } else {
            throw RMAPHandlerException(RMAPHandlerException::LowerException);
          }
        }
        usleep(100);
      }
    }
  }

 private:
  std::shared_ptr<SpaceWireIFOverUART> spwif_{};
  std::string deviceName_{};
};

#endif
