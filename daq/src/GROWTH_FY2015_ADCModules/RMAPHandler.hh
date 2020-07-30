#ifndef RMAPHANDLER_HH_
#define RMAPHANDLER_HH_

#include "CxxUtilities/CxxUtilities.hh"
#include "SpaceWireRMAPLibrary/RMAP.hh"
#include "SpaceWireRMAPLibrary/SpaceWire.hh"

#include <fstream>
#include <iostream>
#include <vector>

/** Wraps RMAPIntiiator for SpaceWire ADC Board applications.
 * The original (old) version, which is no longer maintained,
 * can be found in the SpaceWireRMAPLibrary.
 */
class RMAPHandler {
 protected:
  RMAPTargetNodeDB rmapTargetDB;
  std::string ipAddress;
  u32 tcpPortNumber;

  SpaceWireIFOverTCP* spwif;
  RMAPEngine* rmapEngine;
  RMAPInitiator* rmapInitiator;
  RMAPTargetNode* adcRMAPTargetNode;

  f64 timeOutDuration;
  int maxNTrials;
  bool useDraftECRC = false;
  bool _isConnectedToSpWGbE;

 public:
  static constexpr f64 DefaultTimeOut = 1000;  // ms

 public:
  RMAPHandler(std::string ipAddress, u32 tcpPortNumber = 10030,
              std::vector<RMAPTargetNode*> rmapTargetNodes = {}) {
    using namespace std;
    this->ipAddress = ipAddress;
    this->tcpPortNumber = tcpPortNumber;
    this->timeOutDuration = 1000.0;
    this->maxNTrials = 10;
    this->useDraftECRC = false;
    this->spwif = NULL;
    this->rmapEngine = NULL;
    this->rmapInitiator = NULL;
    this->setTimeOutDuration(DefaultTimeOut);

    // add RMAPTargetNodes to DB
    for (auto& rmapTargetNode : rmapTargetNodes) {
      rmapTargetDB.addRMAPTargetNode(rmapTargetNode);
    }

    adcRMAPTargetNode = rmapTargetDB.getRMAPTargetNode("ADCBox");
    if (adcRMAPTargetNode == nullptr) {
      cerr << "RMAPHandler::RMAPHandler(): ADCBox RMAP Target Node not found" << endl;
      exit(-1);
    }
  }

 protected:
  RMAPHandler() {}

 public:
  virtual ~RMAPHandler() {}

 public:
  bool isConnectedToSpWGbE() const { return _isConnectedToSpWGbE; }

 public:
  virtual bool connectoToSpaceWireToGigabitEther() {
    using namespace std;
    if (spwif != NULL) {
      delete spwif;
    }

    // connect to SpaceWire-to-GigabitEther
    spwif = new SpaceWireIFOverTCP(ipAddress, tcpPortNumber);
    try {
      spwif->open();
      _isConnectedToSpWGbE = true;
    } catch (...) {
      cout << "RMAPHandler::connectoToSpaceWireToGigabitEther() connection failed" << endl;
      _isConnectedToSpWGbE = false;
      return false;
    }

    // start RMAPEngine
    rmapEngine = new RMAPEngine(spwif);
    if (useDraftECRC) {
      cout << "RMAPHandler::connectoToSpaceWireToGigabitEther() setting DraftE CRC mode to RMAPEngine" << endl;
      rmapEngine->setUseDraftECRC(true);
    }
    rmapEngine->start();
    rmapInitiator = new RMAPInitiator(rmapEngine);
    rmapInitiator->setInitiatorLogicalAddress(0xFE);
    rmapInitiator->setVerifyMode(true);
    rmapInitiator->setReplyMode(true);
    if (useDraftECRC) {
      rmapInitiator->setUseDraftECRC(true);
      cout << "RMAPHandler::connectoToSpaceWireToGigabitEther() setting DraftE CRC mode to RMAPInitiator" << endl;
    }

    return true;
  }

 public:
  virtual void disconnectSpWGbE() {
    _isConnectedToSpWGbE = false;

    using namespace std;
    cout << "RMAPHandler::disconnectSpWGbE(): Closing SpaceWire interface" << endl;
    spwif->close();
    cout << "RMAPHandler::disconnectSpWGbE(): Stopping RMAPEngine" << endl;
    rmapEngine->stop();

    cout << "RMAPHandler::disconnectSpWGbE(): Deleting instances" << endl;
    delete rmapEngine;
    delete rmapInitiator;
    delete spwif;

    spwif = NULL;
    rmapEngine = NULL;
    rmapInitiator = NULL;
    cout << "RMAPHandler::disconnectSpWGbE(): Completed" << endl;
  }

 public:
  std::string getIPAddress() const { return ipAddress; }

 public:
  u32 getIPPortNumber() const { return tcpPortNumber; }

 public:
  RMAPInitiator* getRMAPInitiator() const { return rmapInitiator; }

 public:
  void setDraftECRC(bool useECRC = true) {
    useDraftECRC = useECRC;
    if (rmapInitiator != NULL && rmapEngine != NULL) {
      rmapEngine->setUseDraftECRC(useDraftECRC);
      rmapInitiator->setUseDraftECRC(useDraftECRC);
    }
  }

 public:
  void setTimeOutDuration(f64 msec) { timeOutDuration = msec; }

 public:
  virtual void read(std::string rmapTargetNodeID, u32 memoryAddress, u32 length, u8* buffer) {
    RMAPTargetNode* targetNode;
    try {
      targetNode = rmapTargetDB.getRMAPTargetNode(rmapTargetNodeID);
    } catch (...) {
      throw RMAPHandlerException(RMAPHandlerException::NoSuchTarget);
    }
    read(targetNode, memoryAddress, length, buffer);
  }

 public:
  virtual void read(std::string rmapTargetNodeID, std::string memoryObjectID, u8* buffer) {
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
  virtual u32 read32BitRegister(const RMAPTargetNode* rmapTargetNode, u32 memoryAddress) const {
    u8 buffer[4];
    using namespace std;
    if (rmapInitiator == nullptr) {
      return 0x00;
    }
    for (size_t i = 0; i < maxNTrials; i++) {
      try {
        // Lower 16 bits
        rmapInitiator->read(rmapTargetNode, memoryAddress, 2, buffer + 2, timeOutDuration);
        // Upper 16 bits
        rmapInitiator->read(rmapTargetNode, memoryAddress + 2, 2, buffer, timeOutDuration);
        u32 result = (buffer[0] << 24) + (buffer[1] << 16) + (buffer[2] << 8) + (buffer[3]);
        return result;
        break;
      } catch (RMAPInitiatorException& e) {
        cerr << "RMAPHandler::read(): RMAPInitiatorException::" << e.toString() << endl;
        cerr << "Read timed out (address="
             << "0x" << hex << right << setw(8) << setfill('0') << memoryAddress << " length=" << dec << 2
             << "); trying again..." << endl;
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
    return 0x00;
  }

 public:
  virtual void read(RMAPTargetNode* rmapTargetNode, u32 memoryAddress, u32 length, u8* buffer) {
    using namespace std;
    if (rmapInitiator == nullptr) {
      return;
    }
    for (size_t i = 0; i < maxNTrials; i++) {
      try {
        rmapInitiator->read(rmapTargetNode, memoryAddress, length, buffer, timeOutDuration);
        break;
      } catch (RMAPInitiatorException& e) {
        cerr << "RMAPHandler::read(): RMAPInitiatorException::" << e.toString() << endl;
        cerr << "Read timed out (address="
             << "0x" << hex << right << setw(8) << setfill('0') << memoryAddress << " length=" << dec << length
             << "); trying again..." << endl;
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
  virtual void read(RMAPTargetNode* rmapTargetNode, std::string memoryObjectID, u8* buffer) {
    using namespace std;
    if (rmapInitiator == nullptr) {
      return;
    }
    for (size_t i = 0; i < maxNTrials; i++) {
      try {
        rmapInitiator->read(rmapTargetNode, memoryObjectID, buffer, timeOutDuration);
        break;
      } catch (RMAPInitiatorException& e) {
        cerr << "RMAPHandler::read(): RMAPInitiatorException::" << e.toString() << endl;
        std::cerr << "Time out; trying again..." << std::endl;
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
  virtual void write(std::string rmapTargetNodeID, u32 memoryAddress, const u8* data, size_t length) {
    RMAPTargetNode* targetNode;
    try {
      targetNode = rmapTargetDB.getRMAPTargetNode(rmapTargetNodeID);
    } catch (...) {
      throw RMAPHandlerException(RMAPHandlerException::NoSuchTarget);
    }
    write(targetNode, memoryAddress, data, length);
  }

 public:
  virtual void write(RMAPTargetNode* rmapTargetNode, u32 memoryAddress, const u8* data, size_t length) {
    if (rmapInitiator == nullptr) {
      return;
    }
    for (size_t i = 0; i < maxNTrials; i++) {
      try {
        if (length != 0) {
          // TODO: Remove const_cast when SpaceWireRMAPLibrary is updated to accept const u8*.
          rmapInitiator->write(rmapTargetNode, memoryAddress, const_cast<u8*>(data), length, timeOutDuration);
        } else {
          rmapInitiator->write(rmapTargetNode, memoryAddress, nullptr, 0, timeOutDuration);
        }
        break;
      } catch (RMAPInitiatorException& e) {
        std::cerr << "Time out; trying again..." << std::endl;
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
  virtual void write(RMAPTargetNode* rmapTargetNode, std::string memoryObjectID, const u8* data) {
    if (rmapInitiator == nullptr) {
      return;
    }
    for (size_t i = 0; i < maxNTrials; i++) {
      try {
        // TODO: Remove const_cast when SpaceWireRMAPLibrary is updated to accept const u8*.
        rmapInitiator->write(rmapTargetNode, memoryObjectID, const_cast<u8*>(data), timeOutDuration);
      } catch (RMAPInitiatorException& e) {
        std::cerr << "Time out; trying again..." << std::endl;
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
  RMAPTargetNode* getRMAPTargetNode(std::string rmapTargetNodeID) {
    RMAPTargetNode* targetNode;
    try {
      targetNode = rmapTargetDB.getRMAPTargetNode(rmapTargetNodeID);
    } catch (...) {
      throw RMAPHandlerException(RMAPHandlerException::NoSuchTarget);
    }
    return targetNode;
  }

 public:
  virtual void setRegister(u32 address, u16 data) {
    const u8 writeData[2] = {static_cast<u8>(data / 0x100), static_cast<u8>(data % 0x100)};
    this->write(adcRMAPTargetNode, address, writeData, 2);
  }

 public:
  virtual u16 getRegister(u32 address) {
    u8 readData[2];
    this->read(adcRMAPTargetNode, address, 2, readData);
    return (static_cast<u16>(readData[0]) << 16) + readData[1];
  }

  /*
   public:
   RMAPMemoryObject* getMemoryObject(std::string rmapTargetNodeID, std::string memoryObjectID) {
   RMAPTargetNode* targetNode = getRMAPTargetNode(rmapTargetNodeID);
   return targetNode->getMemoryObject(memoryObjectID);
   }*/

 public:
  RMAPEngine* getRMAPEngine() { return rmapEngine; }

 public:
  class RMAPHandlerException {
   public:
    RMAPHandlerException(int type) {
      std::string message;
      switch (type) {
        case NoSuchFile:
          message = "NoSuchFile";
          break;
        case TargetLoadFailed:
          message = "TargetLoadFailed";
          break;
        case NoSuchTarget:
          message = "NoSuchTarget";
          break;
        case TimeOut:
          message = "TimeOut";
          break;
        case CouldNotConnect:
          message = "CouldNotConnect";
          break;
        case LowerException:
          message = "LowerException";
          break;
        default:
          break;
      }
      std::cout << message << std::endl;
    }

    enum { NoSuchFile, TargetLoadFailed, NoSuchTarget, TimeOut, CouldNotConnect, LowerException };
  };
};

#endif
