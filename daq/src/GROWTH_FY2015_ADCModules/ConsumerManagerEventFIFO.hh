/*
 * ConsumerManagerEventFIFO.hh
 *
 *  Created on: June 10 2015
 *      Author: yuasa
 */

#ifndef CONSUMERMANAGEREVENTFIFO_HH_
#define CONSUMERMANAGEREVENTFIFO_HH_

#include "CxxUtilities/CxxUtilities.hh"
#include "GROWTH_FY2015_ADCModules/RMAPHandler.hh"

/** A class which represents ConsumerManager module in the VHDL logic.
 * It also holds information on a ring buffer constructed on SDRAM.
 */
class ConsumerManagerEventFIFO {
 public:
  class ConsumerManagerEventFIFODumpThread : public CxxUtilities::StoppableThread {
   private:
    ConsumerManagerEventFIFO* parent;
    bool dumpEnabled = true;

   public:
    static constexpr double DumpPeriodInMilliSecond = 1000.0;  // ms

   public:
    ConsumerManagerEventFIFODumpThread(ConsumerManagerEventFIFO* parent) { this->parent = parent; }

   public:
    void run() {
      using namespace std;
      CxxUtilities::Condition c;
      while (!stopped) {
        if (dumpEnabled) {
          cout << "ConsumerManagerEventFIFO: Total " << dec << parent->receivedBytes << " bytes received" << endl;
        }
        c.wait(DumpPeriodInMilliSecond);
      }
    }

   public:
    void enableDump() { dumpEnabled = true; }

   public:
    void disableDump() { dumpEnabled = false; }
  };

 public:
  size_t receivedBytes = 0;

 public:
  // Addresses of Consumer Manager Module
  static const uint32_t InitialAddressOf_ConsumerMgr = 0x01010000;
  static const uint32_t ConsumerMgrBA = InitialAddressOf_ConsumerMgr;
  static const uint32_t AddressOf_EventOutputDisableRegister = ConsumerMgrBA + 0x0100;
  static const uint32_t AddressOf_GateSize_FastGate_Register = ConsumerMgrBA + 0x010e;
  static const uint32_t AddressOf_GateSize_SlowGate_Register = ConsumerMgrBA + 0x0110;
  static const uint32_t AddressOf_NumberOf_BaselineSample_Register = ConsumerMgrBA + 0x0112;
  static const uint32_t AddressOf_ConsumerMgr_ResetRegister = ConsumerMgrBA + 0x0114;
  static const uint32_t AddressOf_EventPacket_NumberOfWaveform_Register = ConsumerMgrBA + 0x0116;
  static const uint32_t AddressOf_EventPacket_WaveformDownSampling_Register = ConsumerMgrBA + 0xFFFF;

  // Addresses of EventFIFO
  static const uint32_t InitialAddressOf_EventFIFO = 0x10000000;
  static const uint32_t FinalAddressOf_EventFIFO = 0x1000FFFF;
  static const uint32_t AddressOf_EventFIFO_DataCount_Register = 0x20000000;

 private:
  ConsumerManagerEventFIFODumpThread* dumpThread;

 private:
  RMAPHandler* rmapHandler;
  // RMAPInitiator* rmapInitiator;
  RMAPTargetNode* adcRMAPTargetNode;

 private:
  static const size_t EventFIFOSizeInBytes = 2 * 16 * 1024;  // 16-bit wide * 16-k depth

 public:
  /** Constructor.
   * @param[in] rmapHandler RMAPHandlerUART
   * @param[in] adcRMAPTargetNode RMAPTargetNode that corresponds to the ADC board
   */
  ConsumerManagerEventFIFO(RMAPHandler* rmapHandler, RMAPTargetNode* adcRMAPTargetNode) {
    this->rmapHandler = rmapHandler;
    this->adcRMAPTargetNode = adcRMAPTargetNode;

    dumpThread = new ConsumerManagerEventFIFODumpThread(this);
    // dumpThread->start();
  }

 public:
  virtual ~ConsumerManagerEventFIFO() {
    closeSocket();
    this->stopDumpThread();
  }

 public:
  /** Resets ConsumerManager module.
   */
  void reset() {
    using namespace std;
    if (Debug::consumermanager()) {
      cout << "ConsumerManager::reset()...";
    }
    rmapHandler->setRegister(AddressOf_ConsumerMgr_ResetRegister, 0x0001);
    if (Debug::consumermanager()) {
      cout << "done" << endl;
    }
  }

 private:
  std::vector<uint8_t> receiveBuffer;
  static const size_t ReceiveBufferSize = 3000;

 public:
  /** Retrieve data stored in the EventFIFO.
   * @param maximumsize maximum data size to be returned (in bytes)
   */
  std::vector<uint8_t> getEventData(uint32_t maximumsize = 4000) throw(RMAPInitiatorException) {
    using namespace std;

    receiveBuffer.resize(ReceiveBufferSize);

    // open rmapInitiator if necessary
    /*
     if (rmapInitiator == NULL) {
     try {
     cerr << "ConsumerManagerEventFIFO::read(): trying to open a TCP rmapInitiator" << endl;
     this->openSocket();
     cerr << "ConsumerManagerEventFIFO::read(): rmapInitiator connected" << endl;
     } catch (RMAPInitiatorException& e) {
     if (e.getStatus() == RMAPInitiatorException::Timeout) {
     cerr << "ConsumerManagerEventFIFO::read(): timeout" << endl;
     } else {
     cerr << "ConsumerManagerEventFIFO::read(): TCPSocketException " << e.toString() << endl;
     }
     throw e;
     }
     }
     */

    // read event fifo
    try {
      if (Debug::consumermanager()) {
        cout << "ConsumerManagerEventFIFO::getEventData(): trying to receive data" << endl;
      }
      size_t receivedSize = this->readEventFIFO(&(receiveBuffer[0]), ReceiveBufferSize);
      if (Debug::consumermanager()) {
        cout << "ConsumerManagerEventFIFO::getEventData(): received " << receivedSize << " bytes" << endl;
      }
      // if odd byte is received, wait until the following 1 byte is received.
      if (receivedSize % 2 == 1) {
        cout << "ConsumerManagerEventFIFO::getEventData(): odd bytes. wait for another byte." << endl;
        size_t receivedSizeOneByte = 0;
        while (receivedSizeOneByte == 0) {
          try {
            receivedSizeOneByte = this->readEventFIFO(&(receiveBuffer[receivedSize]), 1);
          } catch (...) {
            cerr << "ConsumerManagerEventFIFO::getEventData(): receive 1 byte timeout. continues." << endl;
          }
        }
        // increment by 1 to make receivedSize even
        receivedSize++;
      }
      receiveBuffer.resize(receivedSize);
    } catch (RMAPInitiatorException& e) {
      if (e.getStatus() == RMAPInitiatorException::Timeout) {
        cerr << "ConsumerManagerEventFIFO::getEventData(): timeout" << endl;
        receiveBuffer.resize(0);
      } else {
        cerr << "ConsumerManagerEventFIFO::getEventData(): TCPSocketException on receive()" << e.toString() << endl;
        throw e;
      }
    }

    // return result
    receivedBytes += receiveBuffer.size();
    return receiveBuffer;
  }

 public:
  void openSocket() throw(RMAPInitiatorException) {
    /*
     using namespace std;
     rmapInitiator = new RMAPInitiator(rmapHandler->getRMAPEngine());
     try {
     cout << "RMAPInitiator instantiated" << endl;
     if (Debug::consumermanager()) {
     cout << "ConsumerManagerEventFIFO::openSocket(): rmapInitiator opened" << endl;
     }
     //receive garbage data
     size_t receivedBytes = 1;
     size_t totalReceivedBytes = 0;
     const size_t temporaryBufferSize = EventFIFOSizeInBytes;
     uint8_t* temporaryBuffer = new uint8_t[temporaryBufferSize];
     try {
     cout << "Receiving garbage data from the last measurement" << endl;
     while (receivedBytes != 0) {
     receivedBytes = 0;
     receivedBytes = this->readEventFIFO(temporaryBuffer, temporaryBufferSize);
     totalReceivedBytes += receivedBytes;
     cout << "Received " << receivedBytes << " bytes" << endl;
     }
     } catch (...) {
     cout << "Read garbage data completed (read " << totalReceivedBytes << " bytes)" << endl;
     }
     delete temporaryBuffer;
     } catch (RMAPInitiatorException& e) {
     cerr << e.toString() << endl;
     exit(-1);
     }*/
  }

 private:
  size_t readEventFIFO(uint8_t* buffer, size_t length) throw(RMAPInitiatorException) {
    const size_t dataCountInBytes = readEventFIFODataCount() * 2;
    const size_t readSize = std::min(dataCountInBytes, length);
    if (readSize != 0) {
      rmapHandler->read(adcRMAPTargetNode, InitialAddressOf_EventFIFO, readSize, buffer);
    }
    return readSize;
  }

 private:
  uint16_t readEventFIFODataCount() throw(RMAPInitiatorException) {
    return this->rmapHandler->getRegister(AddressOf_EventFIFO_DataCount_Register);
  }

 public:
  void closeSocket() {
    /*
     using namespace std;
     if (rmapInitiator != NULL) {
     cerr << "ConsumerManagerEventFIFO::closeSocket(): rmapInitiator closed" << endl;
     delete rmapInitiator;
     rmapInitiator = NULL;
     }*/
  }

 public:
  void enableStatusDump() { dumpThread->enableDump(); }

 public:
  void disableStatusDump() { dumpThread->disableDump(); }

 public:
  /** Enables event data output to EventFIFO.
   */
  void enableEventDataOutput() {
    using namespace std;
    if (Debug::consumermanager()) {
      cout << "Enabling event data output...";
    }
    rmapHandler->setRegister(AddressOf_EventOutputDisableRegister, 0x0000);
    if (Debug::consumermanager()) {
      uint16_t eventOutputDisableRegister = rmapHandler->getRegister(AddressOf_EventOutputDisableRegister);
      cout << "done(disableRegister=" << hex << setw(4) << eventOutputDisableRegister << dec << ")" << endl;
    }
  }

 public:
  /** Disables event data output to EventFIFO.
   * This is used to temporarily stop event data output via
   * TCP/IP, and force processed event data to be buffered
   * in the CosumerManager module. By doing these, the user application
   * software can handle the memory usage especially in high event rate
   * case.
   */
  void disableEventDataOutput() {
    using namespace std;
    if (Debug::consumermanager()) {
      cout << "Disabling event data output...";
    }
    rmapHandler->setRegister(AddressOf_EventOutputDisableRegister, 0x0001);
    if (Debug::consumermanager()) {
      uint16_t eventOutputDisableRegister = rmapHandler->getRegister(AddressOf_EventOutputDisableRegister);
      cout << "done(disableRegister=" << hex << setw(4) << eventOutputDisableRegister << dec << ")" << endl;
    }
  }

 public:
  void startDumpThread() {
    using namespace std;
    if (Debug::consumermanager()) {
      cout << "ConsumerManagerEventFIFO::stopDumpThread(): starting status dump thread" << endl;
    }
    dumpThread->start();
    if (Debug::consumermanager()) {
      cout << "ConsumerManagerEventFIFO::stopDumpThread(): status dump thread started" << endl;
    }
  }

 public:
  void stopDumpThread() {
    using namespace std;
    if (Debug::consumermanager()) {
      cout << "ConsumerManagerEventFIFO::stopDumpThread(): stopping status dump thread" << endl;
    }
    dumpThread->stop();
    if (Debug::consumermanager()) {
      cout << "ConsumerManagerEventFIFO::stopDumpThread(): waiting for status dump thread to actually stop" << endl;
    }
    dumpThread->waitUntilRunMethodComplets();
    if (Debug::consumermanager()) {
      cout << "ConsumerManagerEventFIFO::stopDumpThread(): status dump thread stopped" << endl;
    }
  }

 public:
  /** Sets EventPacket_NumberOfWaveform_Register
   * @param numberofsamples number of data points to be recorded in an event packet
   */
  virtual void setEventPacket_NumberOfWaveform(uint16_t nSamples) {
    using namespace std;
    if (Debug::consumermanager()) {
      cout << "ConsumerManagerEventFIFO::setEventPacket_NumberOfWaveform(" << nSamples << ")...";
    }
    rmapHandler->setRegister(AddressOf_EventPacket_NumberOfWaveform_Register, nSamples);

    if (Debug::consumermanager()) {
      cout << "done" << endl;
      uint16_t nSamplesRead = rmapHandler->getRegister(AddressOf_EventPacket_NumberOfWaveform_Register);
      cout << "readdata[0]:" << (uint32_t)nSamplesRead << endl;
    }
  }
};

#endif /* CONSUMERMANAGEREVENTFIFO_HH_ */
