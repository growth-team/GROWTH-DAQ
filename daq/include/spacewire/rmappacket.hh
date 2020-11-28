#ifndef SPACEWIRE_RMAPPACKET_HH_
#define SPACEWIRE_RMAPPACKET_HH_

#include "spacewire/rmapprotocol.hh"
#include "spacewire/rmapreplystatus.hh"
#include "spacewire/rmaptargetnode.hh"
#include "spacewire/rmaputilities.hh"
#include "spacewire/spacewirepacket.hh"
#include "spacewire/spacewireprotocol.hh"
#include "spacewire/spacewireutilities.hh"

class RMAPPacketException {
 public:
  enum {
    ProtocolIDIsNotRMAP,
    PacketInterpretationFailed,
    InsufficientBufferSize,
    InvalidHeaderCRC,
    InvalidDataCRC,
    DataLengthMismatch
  };

 public:
  RMAPPacketException(u32 status) : CxxUtilities::Exception(status) {}

 public:
  virtual ~RMAPPacketException() = default;

 public:
  virtual std::string toString() {
    // return ClassInformation::demangle(typeid(*this).name());
    using namespace std;
    stringstream ss;
    switch (status) {
      case ProtocolIDIsNotRMAP:
        ss << "ProtocolIDIsNotRMAP";
        break;
      case PacketInterpretationFailed:
        ss << "PacketInterpretationFailed";
        break;
      case InsufficientBufferSize:
        ss << "InsufficientBufferSize";
        break;
      case InvalidHeaderCRC:
        ss << "InvalidHeaderCRC";
        break;
      case InvalidDataCRC:
        ss << "InvalidDataCRC";
        break;
      case DataLengthMismatch:
        ss << "DataLengthMismatch";
        break;
      default:
        ss << "Undefined exception";
        break;
    }
    return ss.str();
  }
};

class RMAPPacket : public SpaceWirePacket {
 public:
  RMAPPacket() : protocolId_(RMAPProtocol::ProtocolIdentifier) {}

  void constructHeader() {
    using namespace std;

    header.clear();
    if (isCommand()) {
      // if command packet
      header.push_back(targetLogicalAddress);
      header.push_back(protocolID_);
      header.push_back(instruction);
      header.push_back(key);
      std::vector<u8> tmpvector;
      u8 tmporaryCounter = replyAddress.size();
      while (tmporaryCounter % 4 != 0) {
        header.push_back(0x00);
        tmporaryCounter++;
      }
      for (size_t i = 0; i < replyAddress.size(); i++) {
        header.push_back(replyAddress.at(i));
      }
      header.push_back(initiatorLogicalAddress);
      header.push_back((u8)((((((transactionID & 0xff00) >> 8))))));
      header.push_back((u8)((((((transactionID & 0x00ff) >> 0))))));
      header.push_back(extendedAddress);
      header.push_back((u8)((((((address & 0xff000000) >> 24))))));
      header.push_back((u8)((((((address & 0x00ff0000) >> 16))))));
      header.push_back((u8)((((((address & 0x0000ff00) >> 8))))));
      header.push_back((u8)((((((address & 0x000000ff) >> 0))))));
      header.push_back((u8)((((((dataLength & 0x00ff0000) >> 16))))));
      header.push_back((u8)((((((dataLength & 0x0000ff00) >> 8))))));
      header.push_back((u8)((((((dataLength & 0x000000ff) >> 0))))));
    } else {
      // if reply packet
      header.push_back(initiatorLogicalAddress);
      header.push_back(protocolID_);
      header.push_back(instruction);
      header.push_back(status);
      header.push_back(targetLogicalAddress);
      header.push_back((u8)((((((transactionID & 0xff00) >> 8))))));
      header.push_back((u8)((((((transactionID & 0x00ff) >> 0))))));
      if (isRead()) {
        header.push_back(0);
        header.push_back((u8)((((((dataLength & 0x00ff0000) >> 16))))));
        header.push_back((u8)((((((dataLength & 0x0000ff00) >> 8))))));
        header.push_back((u8)((((((dataLength & 0x000000ff) >> 0))))));
      }
    }

    if (headerCRCMode == RMAPPacket::AutoCRC) {
      headerCRC = RMAPUtilities::calculateCRC(header);
    }
    header.push_back(headerCRC);
  }

  void constructPacket() {
    constructHeader();
    if (dataCRCMode == RMAPPacket::AutoCRC) {
      dataCRC = RMAPUtilities::calculateCRC(data);
    }
    wholePacket.clear();
    if (isCommand()) {
      SpaceWireUtilities::concatenateTo(wholePacket, targetSpaceWireAddress);
    } else {
      SpaceWireUtilities::concatenateTo(wholePacket, replyAddress);
    }
    SpaceWireUtilities::concatenateTo(wholePacket, header);
    SpaceWireUtilities::concatenateTo(wholePacket, data);
    if (hasData()) {
      wholePacket.push_back(dataCRC);
    }
  }

  void interpretAsAnRMAPPacket(const u8* packet, size_t length) {
    if (length < 8) {
      throw(RMAPPacketException(RMAPPacketException::PacketInterpretationFailed));
    }
    std::vector<u8> temporaryPathAddress{};
    try {
      size_t i = 0;
      size_t rmapIndex = 0;
      size_t rmapIndexAfterSourcePathAddress = 0;
      size_t dataIndex = 0;
      temporaryPathAddress.clear();
      while (packet[i] < 0x20) {
        temporaryPathAddress.push_back(packet[i]);
        i++;
        if (i >= length) {
          throw(RMAPPacketException(RMAPPacketException::PacketInterpretationFailed));
        }
      }

      rmapIndex = i;
      if (packet[rmapIndex + 1] != RMAPProtocol::ProtocolIdentifier) {
        throw(RMAPPacketException(RMAPPacketException::ProtocolIDIsNotRMAP));
      }
      using namespace std;

      instruction = packet[rmapIndex + 2];
      u8 replyPathAddressLength = getReplyPathAddressLength();
      if (isCommand()) {
        // if command packet
        setTargetSpaceWireAddress(temporaryPathAddress);
        setTargetLogicalAddress(packet[rmapIndex]);
        setKey(packet[rmapIndex + 3]);
        vector<u8> temporaryReplyAddress;
        for (u8 i = 0; i < replyPathAddressLength * 4; i++) {
          temporaryReplyAddress.push_back(packet[rmapIndex + 4 + i]);
        }
        setReplyAddress(temporaryReplyAddress);
        rmapIndexAfterSourcePathAddress = rmapIndex + 4 + replyPathAddressLength * 4;
        setInitiatorLogicalAddress(packet[rmapIndexAfterSourcePathAddress + 0]);
        u8 uppertid, lowertid;
        uppertid = packet[rmapIndexAfterSourcePathAddress + 1];
        lowertid = packet[rmapIndexAfterSourcePathAddress + 2];
        setTransactionID((u32)(((uppertid * 0x100 + lowertid))));
        setExtendedAddress(packet[rmapIndexAfterSourcePathAddress + 3]);
        u8 address_3, address_2, address_1, address_0;
        address_3 = packet[rmapIndexAfterSourcePathAddress + 4];
        address_2 = packet[rmapIndexAfterSourcePathAddress + 5];
        address_1 = packet[rmapIndexAfterSourcePathAddress + 6];
        address_0 = packet[rmapIndexAfterSourcePathAddress + 7];
        setAddress(address_3 * 0x01000000 + address_2 * 0x00010000 + address_1 * 0x00000100 + address_0 * 0x00000001);
        u8 length_2, length_1, length_0;
        u32 lengthSpecifiedInPacket;
        length_2 = packet[rmapIndexAfterSourcePathAddress + 8];
        length_1 = packet[rmapIndexAfterSourcePathAddress + 9];
        length_0 = packet[rmapIndexAfterSourcePathAddress + 10];
        lengthSpecifiedInPacket = length_2 * 0x010000 + length_1 * 0x000100 + length_0 * 0x000001;
        setDataLength(lengthSpecifiedInPacket);
        u8 temporaryHeaderCRC = packet[rmapIndexAfterSourcePathAddress + 11];
        if (headerCRCIsChecked == true) {
          u32 headerCRCMode_original = headerCRCMode;
          headerCRCMode = RMAPPacket::AutoCRC;
          constructHeader();
          headerCRCMode = headerCRCMode_original;
          if (headerCRC != temporaryHeaderCRC) {
            throw(RMAPPacketException(RMAPPacketException::InvalidHeaderCRC));
          }
        } else {
          headerCRC = temporaryHeaderCRC;
        }
        dataIndex = rmapIndexAfterSourcePathAddress + 12;
        data.clear();
        if (isWrite()) {
          for (u32 i = 0; i < lengthSpecifiedInPacket; i++) {
            if ((dataIndex + i) < (length - 1)) {
              data.push_back(packet[dataIndex + i]);
            } else {
              throw(RMAPPacketException(RMAPPacketException::DataLengthMismatch));
            }
          }

          // length check for DataCRC
          u8 temporaryDataCRC = 0x00;
          if ((dataIndex + lengthSpecifiedInPacket) == (length - 1)) {
            temporaryDataCRC = packet[dataIndex + lengthSpecifiedInPacket];
          } else {
            throw(RMAPPacketException(RMAPPacketException::DataLengthMismatch));
          }
          dataCRC = RMAPUtilities::calculateCRC(data);
          if (dataCRCIsChecked == true) {
            if (dataCRC != temporaryDataCRC) {
              throw(RMAPPacketException(RMAPPacketException::InvalidDataCRC));
            }
          } else {
            dataCRC = temporaryDataCRC;
          }
        }

      } else {
        // if reply packet
        setReplyAddress(temporaryPathAddress, false);
        setInitiatorLogicalAddress(packet[rmapIndex]);
        setStatus(packet[rmapIndex + 3]);
        setTargetLogicalAddress(packet[rmapIndex + 4]);
        u8 uppertid, lowertid;
        uppertid = packet[rmapIndex + 5];
        lowertid = packet[rmapIndex + 6];
        setTransactionID((u32)(((uppertid * 0x100 + lowertid))));
        if (isWrite()) {
          u8 temporaryHeaderCRC = packet[rmapIndex + 7];
          u32 headerCRCMode_original = headerCRCMode;
          headerCRCMode = RMAPPacket::AutoCRC;
          constructHeader();
          headerCRCMode = headerCRCMode_original;
          if (headerCRCIsChecked == true) {
            if (headerCRC != temporaryHeaderCRC) {
              throw(RMAPPacketException(RMAPPacketException::InvalidHeaderCRC));
            }
          } else {
            headerCRC = temporaryHeaderCRC;
          }
        } else {
          u8 length_2, length_1, length_0;
          u32 lengthSpecifiedInPacket;
          length_2 = packet[rmapIndex + 8];
          length_1 = packet[rmapIndex + 9];
          length_0 = packet[rmapIndex + 10];
          lengthSpecifiedInPacket = length_2 * 0x010000 + length_1 * 0x000100 + length_0 * 0x000001;
          setDataLength(lengthSpecifiedInPacket);
          u8 temporaryHeaderCRC = packet[rmapIndex + 11];
          constructHeader();
          if (headerCRCIsChecked == true) {
            if (headerCRC != temporaryHeaderCRC) {
              throw(RMAPPacketException(RMAPPacketException::InvalidHeaderCRC));
            }
          } else {
            headerCRC = temporaryHeaderCRC;
          }
          dataIndex = rmapIndex + 12;
          data.clear();
          for (u32 i = 0; i < lengthSpecifiedInPacket; i++) {
            if ((dataIndex + i) < (length - 1)) {
              data.push_back(packet[dataIndex + i]);
            } else {
              dataCRC = 0x00;  // initialized
              throw(RMAPPacketException(RMAPPacketException::DataLengthMismatch));
            }
          }

          // length check for DataCRC
          u8 temporaryDataCRC = 0x00;
          if ((dataIndex + lengthSpecifiedInPacket) == (length - 1)) {
            temporaryDataCRC = packet[dataIndex + lengthSpecifiedInPacket];
          } else {
            throw(RMAPPacketException(RMAPPacketException::DataLengthMismatch));
          }
          dataCRC = RMAPUtilities::calculateCRC(data);
          if (dataCRCIsChecked == true) {
            if (dataCRC != temporaryDataCRC) {
              throw(RMAPPacketException(RMAPPacketException::InvalidDataCRC));
            }
          } else {
            dataCRC = temporaryDataCRC;
          }
        }
      }

    } catch (exception& e) {
      throw(RMAPPacketException(RMAPPacketException::PacketInterpretationFailed));
    }
    u32 previousHeaderCRCMode = headerCRCMode;
    u32 previousDataCRCMode = dataCRCMode;
    headerCRCMode = RMAPPacket::ManualCRC;
    dataCRCMode = RMAPPacket::ManualCRC;
    constructPacket();
    headerCRCMode = previousHeaderCRCMode;
    dataCRCMode = previousDataCRCMode;
  }

  void interpretAsAnRMAPPacket(const std::vector<u8>* data) {
    if (data->empty()) {
      throw RMAPPacketException(RMAPPacketException::PacketInterpretationFailed);
    }
    interpretAsAnRMAPPacket(data->data(), data->size());
  }

  void setRMAPTargetInformation(RMAPTargetNode* rmapTargetNode) {
    setTargetLogicalAddress(rmapTargetNode->getTargetLogicalAddress());
    setReplyAddress(rmapTargetNode->getReplyAddress());
    setTargetSpaceWireAddress(rmapTargetNode->getTargetSpaceWireAddress());
    setKey(rmapTargetNode->getDefaultKey());
    if (rmapTargetNode->isInitiatorLogicalAddressSet()) {
      setInitiatorLogicalAddress(rmapTargetNode->getInitiatorLogicalAddress());
    }
  }

  void setRMAPTargetInformation(RMAPTargetNode& rmapTargetNode) { setRMAPTargetInformation(&rmapTargetNode); }
  void setCommand() { instruction = instruction | RMAPPacket::BitMaskForCommandReply; }
  void setReply() { instruction = instruction & (~RMAPPacket::BitMaskForCommandReply); }
  void setWrite() { instruction = instruction | RMAPPacket::BitMaskForWriteRead; }
  void setRead() { instruction = instruction & (~RMAPPacket::BitMaskForWriteRead); }
  void setVerifyFlag(bool mode) {
    if (mode) {
      instruction = instruction | RMAPPacket::BitMaskForVerifyFlag;
    } else {
      instruction = instruction & (~RMAPPacket::BitMaskForVerifyFlag);
    }
  }

  void setReplyFlag(bool mode) {
    if (mode) {
      instruction = instruction | RMAPPacket::BitMaskForReplyFlag;
    } else {
      instruction = instruction & (~RMAPPacket::BitMaskForReplyFlag);
    }
  }

  void setIncrementFlag(bool mode) {
    if (mode) {
      instruction = instruction | RMAPPacket::BitMaskForIncrementFlag;
    } else {
      instruction = instruction & (~RMAPPacket::BitMaskForIncrementFlag);
    }
  }

  void setReplyPathAddressLength(u8 pathAddressLength) {
    // TODO: verify the following logic
    instruction = (instruction & ~(RMAPPacket::BitMaskForIncrementFlag)) + pathAddressLength &
                  RMAPPacket::BitMaskForIncrementFlag;
  }

  bool isCommand() const { return (instruction & RMAPPacket::BitMaskForCommandReply) != 0; }
  bool isReply() const { return (instruction & RMAPPacket::BitMaskForCommandReply) == 0; }
  bool isWrite() const { return (instruction & RMAPPacket::BitMaskForWriteRead) != 0; }
  bool isRead() const { return (instruction & RMAPPacket::BitMaskForWriteRead) == 0; }
  bool isVerifyFlagSet() const { return (instruction & RMAPPacket::BitMaskForVerifyFlag) != 0; }
  bool isReplyFlagSet() const { return (instruction & RMAPPacket::BitMaskForReplyFlag) != 0; }
  bool isIncrementFlagSet() const { return (instruction & RMAPPacket::BitMaskForIncrementFlag) != 0; }

  u8 getReplyPathAddressLength() const { return instruction & RMAPPacket::BitMaskForReplyPathAddressLength; }

  u32 getAddress() const { return address; }

  bool hasData() const { return !data.empty() || (isCommand() && isWrite()) || (isReply() && isRead()); }

  std::vector<u8> getData() const { return data; }

  void getData(u8* buffer, size_t maxLength) {
    const auto length = data.size();
    if (maxLength < length) {
      throw RMAPPacketException(RMAPPacketException::InsufficientBufferSize);
    }
    for (size_t i = 0; i < length; i++) {
      buffer[i] = data[i];
    }
  }

  void getData(std::vector<u8>* buffer) { *buffer = data; }

  u8* getDataBufferAsArrayPointer() { return (data.size() != 0) ? (u8*)(&data[0]) : NULL; }
  std::vector<u8>* getDataBuffer() { return &data; }
  std::vector<u8>* getDataBufferAsVectorPointer() { return &data; }

  u8 getDataCRC() const { return dataCRC; }
  u32 getDataLength() const { return dataLength; }
  u32 getLength() const { return dataLength; }
  u8 getExtendedAddress() const { return extendedAddress; }
  u8 getHeaderCRC() const { return headerCRC; }
  u8 getInitiatorLogicalAddress() const { return initiatorLogicalAddress; }
  u8 getInstruction() const { return instruction; }
  u8 getKey() const { return key; }
  const std::vector<u8>& getReplyAddress() const { return replyAddress; }
  u16 getTransactionID() const { return transactionID; }
  u8 getStatus() const { return status; }
  u8 getTargetLogicalAddress() const { return targetLogicalAddress; }
  const std::vector<u8>& getTargetSpaceWireAddress() const { return targetSpaceWireAddress; }

  u32 getHeaderCRCMode() const { return headerCRCMode; }
  u32 getDataCRCMode() const { return dataCRCMode; }

  void setAddress(u32 address) { this->address = address; }

  void setData(std::vector<u8>& data) {
    this->data = data;
    this->dataLength = data.size();
  }

  void setData(u8* data, size_t length) {
    this->data.clear();
    for (size_t i = 0; i < length; i++) {
      this->data.push_back(data[i]);
    }
    this->dataLength = length;
  }

  void setDataCRC(u8 dataCRC) { this->dataCRC = dataCRC; }
  void setDataLength(u32 dataLength) { this->dataLength = dataLength; }
  void setLength(u32 dataLength) { this->dataLength = dataLength; }
  void setExtendedAddress(u8 extendedAddress) { this->extendedAddress = extendedAddress; }
  void setHeaderCRC(u8 headerCRC) { this->headerCRC = headerCRC; }
  void setInitiatorLogicalAddress(u8 initiatorLogicalAddress) {
    this->initiatorLogicalAddress = initiatorLogicalAddress;
  }
  void setInstruction(u8 instruction) { this->instruction = instruction; }

  void setKey(u8 key) { this->key = key; }

  void setReplyAddress(std::vector<u8> replyAddress,  //
                       bool automaticallySetPathAddressLengthToInstructionField = true) {
    this->replyAddress = replyAddress;
    if (automaticallySetPathAddressLengthToInstructionField) {
      if (replyAddress.size() % 4 == 0) {
        instruction = (instruction & (~BitMaskForReplyPathAddressLength)) + replyAddress.size() / 4;
      } else {
        instruction = (instruction & (~BitMaskForReplyPathAddressLength)) + (replyAddress.size() + 4) / 4;
      }
    }
  }

  void setTransactionID(u16 transactionID) { this->transactionID = transactionID; }
  void setStatus(u8 status) { this->status = status; }
  void setHeaderCRCMode(u32 headerCRCMode) { this->headerCRCMode = headerCRCMode; }
  void setDataCRCMode(u32 dataCRCMode) { this->dataCRCMode = dataCRCMode; }

  void addData(u8 oneByte) { this->data.push_back(oneByte); }
  void clearData() { data.clear(); }
  void addData(const std::vector<u8>& array) { data.insert(data.end(), array.begin(), array.end()); }

  std::string toString() { return isCommand() ? toStringCommandPacket() : toStringReplyPacket(); }

  void setTargetLogicalAddress(u8 targetLogicalAddress) { this->targetLogicalAddress = targetLogicalAddress; }
  void setTargetSpaceWireAddress(const std::vector<u8>& targetSpaceWireAddress) {
    this->targetSpaceWireAddress = targetSpaceWireAddress;
  }

  std::vector<u8>* getPacketBufferPointer() {
    constructPacket();
    return &wholePacket;
  }

  /** Constructs a reply packet for a corresponding command packet.
   * @param[in] commandPacket a command packet of which reply packet will be constructed
   */
  static RMAPPacket* constructReplyForCommand(RMAPPacket* commandPacket,
                                              u8 status = RMAPReplyStatus::CommandExcecutedSuccessfully) {
    RMAPPacket* replyPacket = new RMAPPacket();
    *replyPacket = *commandPacket;

    // remove leading zeros in the Reply Address (Reply SpaceWire Address)
    replyPacket->setReplyAddress(removeLeadingZerosInReplyAddress(replyPacket->getReplyAddress()));

    replyPacket->clearData();
    replyPacket->setReply();
    replyPacket->setStatus(status);
    return replyPacket;
  }

  bool getDataCRCIsChecked() const { return dataCRCIsChecked; }
  bool getHeaderCRCIsChecked() const { return headerCRCIsChecked; }
  void setDataCRCIsChecked(bool dataCRCIsChecked) { this->dataCRCIsChecked = dataCRCIsChecked; }
  void setHeaderCRCIsChecked(bool headerCRCIsChecked) { this->headerCRCIsChecked = headerCRCIsChecked; }

  enum { AutoCRC, ManualCRC };

  static const u8 BitMaskForReserved = 0x80;
  static const u8 BitMaskForCommandReply = 0x40;
  static const u8 BitMaskForWriteRead = 0x20;
  static const u8 BitMaskForVerifyFlag = 0x10;
  static const u8 BitMaskForReplyFlag = 0x08;
  static const u8 BitMaskForIncrementFlag = 0x04;
  static const u8 BitMaskForReplyPathAddressLength = 0x3;

 private:
  static std::vector<u8> removeLeadingZerosInReplyAddress(std::vector<u8> replyAddress) {
    bool nonZeroValueHasAppeared = false;
    std::vector<u8> result;
    for (size_t i = 0; i < replyAddress.size(); i++) {
      if (nonZeroValueHasAppeared == false && replyAddress[i] != 0x00) {
        nonZeroValueHasAppeared = true;
      }
      if (nonZeroValueHasAppeared) {
        result.push_back(replyAddress[i]);
      }
    }
    return result;
  }

 private:
  std::string toStringCommandPacket() {
    using namespace std;

    stringstream ss;
    ///////////////////////////////
    // Command
    ///////////////////////////////
    // Target SpaceWire Address
    if (targetSpaceWireAddress.size() != 0) {
      ss << "--------- Target SpaceWire Address ---------" << endl;
      SpaceWireUtilities::dumpPacket(&ss, &targetSpaceWireAddress, 1, 128);
    }
    // Header
    ss << "--------- RMAP Header Part ---------" << endl;
    // Initiator Logical Address
    ss << "Initiator Logical Address : 0x" << right << setw(2) << setfill('0') << hex << (u32)(initiatorLogicalAddress)
       << endl;
    // Target Logical Address
    ss << "Target Logic. Address     : 0x" << right << setw(2) << setfill('0') << hex
       << (unsigned int)(targetLogicalAddress) << endl;
    // Protocol Identifier
    ss << "Protocol ID               : 0x" << right << setw(2) << setfill('0') << hex << (unsigned int)(1) << endl;
    // Instruction
    ss << "Instruction               : 0x" << right << setw(2) << setfill('0') << hex << (unsigned int)(instruction)
       << endl;
    toStringInstructionField(ss);
    // Key
    ss << "Key                       : 0x" << setw(2) << setfill('0') << hex << (unsigned int)(key) << endl;
    // Reply Address
    if (replyAddress.size() != 0) {
      ss << "Reply Address             : ";
      SpaceWireUtilities::dumpPacket(&ss, &replyAddress, 1, 128);
    } else {
      ss << "Reply Address         : --none--" << endl;
    }
    ss << "Transaction Identifier    : 0x" << right << setw(4) << setfill('0') << hex << (unsigned int)(transactionID)
       << endl;
    ss << "Extended Address          : 0x" << right << setw(2) << setfill('0') << hex << (unsigned int)(extendedAddress)
       << endl;
    ss << "Address                   : 0x" << right << setw(8) << setfill('0') << hex << (unsigned int)(address)
       << endl;
    ss << "Data Length (bytes)       : 0x" << right << setw(6) << setfill('0') << hex << (unsigned int)(dataLength)
       << " (" << dec << dataLength << "dec)" << endl;
    ss << "Header CRC                : 0x" << right << setw(2) << setfill('0') << hex << (unsigned int)(headerCRC)
       << endl;
    // Data Part
    ss << "---------  RMAP Data Part  ---------" << endl;
    if (isWrite()) {
      ss << "[data size = " << dec << dataLength << "bytes]" << endl;
      SpaceWireUtilities::dumpPacket(&ss, &data, 1, 16);
      ss << "Data CRC                  : " << right << setw(2) << setfill('0') << hex << (unsigned int)(dataCRC)
         << endl;
    } else {
      ss << "--- none ---" << endl;
    }
    ss << endl;
    this->constructPacket();
    ss << "Total data (bytes)        : " << dec << this->getPacketBufferPointer()->size() << endl;
    ss << dec << endl;
    return ss.str();
  }

  std::string toStringReplyPacket() {
    using namespace std;

    stringstream ss;
    this->constructPacket();
    ///////////////////////////////
    // Reply
    ///////////////////////////////
    ss << "RMAP Packet Dump" << endl;
    // Reply Address
    if (replyAddress.size() != 0) {
      ss << "--------- Reply Address ---------" << endl;
      ss << "Reply Address       : ";
      SpaceWireUtilities::dumpPacket(&ss, &replyAddress, 1, 128);
    }
    // Header
    ss << "--------- RMAP Header Part ---------" << endl;
    // Initiator Logical Address
    ss << "Initiator Logical Address : 0x" << right << setw(2) << setfill('0') << hex << (u32)(initiatorLogicalAddress)
       << endl;
    // Target Logical Address
    ss << "Target Logical Address    : 0x" << right << setw(2) << setfill('0') << hex
       << (unsigned int)(targetLogicalAddress) << endl;
    // Protocol Identifier
    ss << "Protocol ID               : 0x" << right << setw(2) << setfill('0') << hex << (unsigned int)(1) << endl;
    // Instruction
    ss << "Instruction               : 0x" << right << setw(2) << setfill('0') << hex << (unsigned int)(instruction)
       << endl;
    toStringInstructionField(ss);
    // Status
    std::string statusstring;
    switch (status) {
      case 0x00:
        statusstring = "Successfully Executed";
        break;
      case 0x01:
        statusstring = "General Error";
        break;
      case 0x02:
        statusstring = "Unused RMAP Packet Type or Command Code";
        break;
      case 0x03:
        statusstring = "Invalid Target Key";
        break;
      case 0x04:
        statusstring = "Invalid Data CRC";
        break;
      case 0x05:
        statusstring = "Early EOP";
        break;
      case 0x06:
        statusstring = "Cargo Too Large";
        break;
      case 0x07:
        statusstring = "EEP";
        break;
      case 0x08:
        statusstring = "Reserved";
        break;
      case 0x09:
        statusstring = "Verify Buffer Overrun";
        break;
      case 0x0a:
        statusstring = "RMAP Command Not Implemented or Not Authorized";
        break;
      case 0x0b:
        statusstring = "Invalid Target Logical Address";
        break;
      default:
        statusstring = "Reserved";
        break;
    }
    ss << "Status                    : 0x" << right << setw(2) << setfill('0') << hex << (unsigned int)(status) << " ("
       << statusstring << ")" << endl;
    ss << "Transaction Identifier    : 0x" << right << setw(4) << setfill('0') << hex << (unsigned int)(transactionID)
       << endl;
    if (isRead()) {
      ss << "Data Length (bytes)       : 0x" << right << setw(6) << setfill('0') << hex << (unsigned int)(dataLength)
         << " (" << dec << dataLength << "dec)" << endl;
    }
    ss << "Header CRC                : 0x" << right << setw(2) << setfill('0') << hex << (unsigned int)(headerCRC)
       << endl;
    // Data Part
    ss << "---------  RMAP Data Part  ---------" << endl;
    if (isRead()) {
      ss << "[data size = " << dec << dataLength << "bytes]" << endl;
      SpaceWireUtilities::dumpPacket(&ss, &data, 1, 128);
      ss << "Data CRC    : 0x" << right << setw(2) << setfill('0') << hex << (unsigned int)(dataCRC) << endl;
    } else {
      ss << "--- none ---" << endl;
    }
    ss << endl;
    this->constructPacket();
    ss << "Total data (bytes)        : " << dec << this->getPacketBufferPointer()->size() << endl;
    ss << dec << endl;
    return ss.str();
  }

  void toStringInstructionField(std::stringstream& ss) {
    using namespace std;

    // packet type (Command or Reply)
    ss << " ------------------------------" << endl;
    ss << " |Reserved    : 0" << endl;
    ss << " |Packet Type : " << (isCommand() ? 1 : 0);
    ss << " " << (isCommand() ? "(Command)" : "(Reply)") << endl;
    // Write or Read
    ss << " |Write/Read  : " << (isWrite() ? 1 : 0);
    ss << " " << (isWrite() ? "(Write)" : "(Read)") << endl;
    // Verify mode
    ss << " |Verify Mode : " << (isVerifyFlagSet() ? 1 : 0);
    ss << " " << (isVerifyFlagSet() ? "(Verify)" : "(No Verify)") << endl;
    // Ack mode
    ss << " |Reply Mode  : " << (isReplyFlagSet() ? 1 : 0);
    ss << " " << (isReplyFlagSet() ? "(Reply)" : "(No Reply)") << endl;
    // Increment
    ss << " |Increment   : " << (isIncrementFlagSet() ? 1 : 0);
    ss << " " << (isIncrementFlagSet() ? "(Increment)" : "(No Increment)") << endl;
    // SPAL
    ss << " |R.A.L.      : " << setw(1) << setfill('0') << hex
       << (u32)((instruction & BitMaskForReplyPathAddressLength)) << endl;
    ss << " |(R.A.L. = Reply Address Length)" << endl;
    ss << " ------------------------------" << endl;
  }

  std::vector<u8> targetSpaceWireAddress{};
  u8 targetLogicalAddress=SpaceWireProtocol::DefaultLogicalAddress;

  u8 instruction{};
  u8 key=RMAPProtocol::DefaultKey;
  std::vector<u8> replyAddress{};
  u8 initiatorLogicalAddress=SpaceWireProtocol::DefaultLogicalAddress;
  u8 extendedAddress=RMAPProtocol::DefaultExtendedAddress;
  u32 address{};
  u32 dataLength{};
  u8 status=RMAPProtocol::DefaultStatus;
  u8 headerCRC{};
  u16 transactionID=RMAPProtocol::DefaultTID;

  std::vector<u8> header{};
  std::vector<u8> data{};
  u8 dataCRC{};

  u32 headerCRCMode= RMAPPacket::AutoCRC;
  u32 dataCRCMode= RMAPPacket::AutoCRC;

  bool headerCRCIsChecked{};
  bool dataCRCIsChecked{};
};
#endif
