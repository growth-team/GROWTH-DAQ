#ifndef SPACEWIRE_RMAPPACKET_HH_
#define SPACEWIRE_RMAPPACKET_HH_

#include "spacewire/rmapprotocol.hh"
#include "spacewire/rmapreplystatus.hh"
#include "spacewire/rmaptargetnode.hh"
#include "spacewire/rmaputilities.hh"
#include "spacewire/spacewirepacket.hh"
#include "spacewire/spacewireutil.hh"

class RMAPPacketException : public std::runtime_error {
 public:
  RMAPPacketException(const std::string& message) : std::runtime_error(message) {}
};

class RMAPPacket : public SpaceWirePacket {
 public:
  RMAPPacket():SpaceWirePacket(RMAPProtocol::ProtocolIdentifier) {
	  //protocolID_ =RMAPProtocol::ProtocolIdentifier;
  }

  void constructHeader() {
    header_.clear();
    if (isCommand()) {
      // if command packet
      header_.push_back(targetLogicalAddress_);
      header_.push_back(protocolID_);
      header_.push_back(instruction_);
      header_.push_back(key_);
      std::vector<u8> tmpvector;
      u8 tmporaryCounter = replyAddress.size();
      while (tmporaryCounter % 4 != 0) {
        header_.push_back(0x00);
        tmporaryCounter++;
      }
      for (size_t i = 0; i < replyAddress.size(); i++) {
        header_.push_back(replyAddress.at(i));
      }
      header_.push_back(initiatorLogicalAddress_);
      header_.push_back((u8)((((((transactionID_ & 0xff00) >> 8))))));
      header_.push_back((u8)((((((transactionID_ & 0x00ff) >> 0))))));
      header_.push_back(extendedAddress_);
      header_.push_back((u8)((((((address_ & 0xff000000) >> 24))))));
      header_.push_back((u8)((((((address_ & 0x00ff0000) >> 16))))));
      header_.push_back((u8)((((((address_ & 0x0000ff00) >> 8))))));
      header_.push_back((u8)((((((address_ & 0x000000ff) >> 0))))));
      header_.push_back((u8)((((((dataLength_ & 0x00ff0000) >> 16))))));
      header_.push_back((u8)((((((dataLength_ & 0x0000ff00) >> 8))))));
      header_.push_back((u8)((((((dataLength_ & 0x000000ff) >> 0))))));
    } else {
      // if reply packet
      header_.push_back(initiatorLogicalAddress_);
      header_.push_back(protocolID_);
      header_.push_back(instruction_);
      header_.push_back(status_);
      header_.push_back(targetLogicalAddress_);
      header_.push_back((u8)((((((transactionID_ & 0xff00) >> 8))))));
      header_.push_back((u8)((((((transactionID_ & 0x00ff) >> 0))))));
      if (isRead()) {
        header_.push_back(0);
        header_.push_back((u8)((((((dataLength_ & 0x00ff0000) >> 16))))));
        header_.push_back((u8)((((((dataLength_ & 0x0000ff00) >> 8))))));
        header_.push_back((u8)((((((dataLength_ & 0x000000ff) >> 0))))));
      }
    }

    if (headerCRCMode_ == RMAPPacket::AutoCRC) {
      headerCRC_ = RMAPUtilities::calculateCRC(header_.data(), header_.size());
    }
    header_.push_back(headerCRC_);
  }

  void constructPacket() {
    constructHeader();
    if (dataCRCMode_ == RMAPPacket::AutoCRC) {
      dataCRC_ = RMAPUtilities::calculateCRC(data_.data(), data_.size());
    }
    wholePacket_.clear();
    if (isCommand()) {
      wholePacket_.insert(wholePacket_.end(), targetSpaceWireAddress_.begin(), targetSpaceWireAddress_.end());
    } else {
      wholePacket_.insert(wholePacket_.end(), replyAddress.begin(), replyAddress.end());
    }
    wholePacket_.insert(wholePacket_.end(), header_.begin(), header_.end());
    wholePacket_.insert(wholePacket_.end(), data_.begin(), data_.end());
    if (hasData()) {
      wholePacket_.push_back(dataCRC_);
    }
  }

  void interpretAsAnRMAPPacket(const u8* packet, size_t length) {
    if (length < 8) {
      throw(RMAPPacketException("packet interpretation failed"));
    }
    try {
      size_t i = 0;
      size_t rmapIndex = 0;
      size_t rmapIndexAfterSourcePathAddress = 0;
      size_t dataIndex = 0;
      temporaryPathAddress_.clear();
      while (packet[i] < 0x20) {
        temporaryPathAddress_.push_back(packet[i]);
        i++;
        if (i >= length) {
          throw(RMAPPacketException("packet too short"));
        }
      }

      rmapIndex = i;
      if(rmapIndex + 8 > length){
        throw(RMAPPacketException("packet too short"));
      }
      if (packet[rmapIndex + 1] != RMAPProtocol::ProtocolIdentifier) {
        throw(RMAPPacketException("non-rmap protocol id"));
      }

      instruction_ = packet[rmapIndex + 2];
      u8 replyPathAddressLength = getReplyPathAddressLength();
      if (isCommand()) {
        // if command packet
        setTargetSpaceWireAddress(temporaryPathAddress_);
        setTargetLogicalAddress(packet[rmapIndex]);
        setKey(packet[rmapIndex + 3]);
        std::vector<u8> temporaryReplyAddress{};
        for (u8 i = 0; i < replyPathAddressLength * 4; i++) {
          temporaryReplyAddress.push_back(packet[rmapIndex + 4 + i]);
        }
        setReplyAddress(temporaryReplyAddress);
        rmapIndexAfterSourcePathAddress = rmapIndex + 4 + replyPathAddressLength * 4;
        setInitiatorLogicalAddress(packet[rmapIndexAfterSourcePathAddress + 0]);

        const u16 uppertid = packet[rmapIndexAfterSourcePathAddress + 1];
        const u16 lowertid = packet[rmapIndexAfterSourcePathAddress + 2];
        const auto tid = (uppertid << 8) + lowertid;
        setTransactionID(tid);
        setExtendedAddress(packet[rmapIndexAfterSourcePathAddress + 3]);
        const u32 address_3 = packet[rmapIndexAfterSourcePathAddress + 4];
        const u32 address_2 = packet[rmapIndexAfterSourcePathAddress + 5];
        const u32 address_1 = packet[rmapIndexAfterSourcePathAddress + 6];
        const u32 address_0 = packet[rmapIndexAfterSourcePathAddress + 7];
        const auto addresss = (address_3 << 24) + (address_2 << 16) + (address_1 << 8) + address_0;
        setAddress(address_);
        const u32 length_2 = packet[rmapIndexAfterSourcePathAddress + 8];
        const u32 length_1 = packet[rmapIndexAfterSourcePathAddress + 9];
        const u32 length_0 = packet[rmapIndexAfterSourcePathAddress + 10];
        const auto lengthSpecifiedInPacket = (length_2 << 16) + (length_1 << 8) + length_0;
        setDataLength(lengthSpecifiedInPacket);
        const u8 temporaryHeaderCRC = packet[rmapIndexAfterSourcePathAddress + 11];
        if (headerCRCIsChecked_) {
          const u32 headerCRCMode_original = headerCRCMode_;
          headerCRCMode_ = RMAPPacket::AutoCRC;
          constructHeader();
          headerCRCMode_ = headerCRCMode_original;
          if (headerCRC_ != temporaryHeaderCRC) {
            throw(RMAPPacketException("invalid header crc"));
          }
        } else {
          headerCRC_ = temporaryHeaderCRC;
        }
        dataIndex = rmapIndexAfterSourcePathAddress + 12;
        data_.clear();
        if (isWrite()) {
          for (u32 i = 0; i < lengthSpecifiedInPacket; i++) {
            if ((dataIndex + i) < (length - 1)) {
              data_.push_back(packet[dataIndex + i]);
            } else {
              throw(RMAPPacketException("data length mismatch"));
            }
          }

          // length check for DataCRC
          u8 temporaryDataCRC = 0x00;
          if ((dataIndex + lengthSpecifiedInPacket) == (length - 1)) {
            temporaryDataCRC = packet[dataIndex + lengthSpecifiedInPacket];
          } else {
            throw(RMAPPacketException("data length mismatch"));
          }
          dataCRC_ = RMAPUtilities::calculateCRC(data_.data(), data_.size());
          if (dataCRCIsChecked_) {
            if (dataCRC_ != temporaryDataCRC) {
              throw(RMAPPacketException("invalid data crc"));
            }
          } else {
            dataCRC_ = temporaryDataCRC;
          }
        }

      } else {
        // if reply packet
        setReplyAddress(temporaryPathAddress_, false);
        setInitiatorLogicalAddress(packet[rmapIndex]);
        setStatus(packet[rmapIndex + 3]);
        setTargetLogicalAddress(packet[rmapIndex + 4]);
        const u16 uppertid = packet[rmapIndex + 5];
        const u16 lowertid = packet[rmapIndex + 6];
        const u16 tid = (uppertid << 8) + lowertid;
        setTransactionID(tid);
        if (isWrite()) {
          const u8 temporaryHeaderCRC = packet[rmapIndex + 7];
          u32 headerCRCMode_original = headerCRCMode_;
          headerCRCMode_ = RMAPPacket::AutoCRC;
          constructHeader();
          headerCRCMode_ = headerCRCMode_original;
          if (headerCRCIsChecked_) {
            if (headerCRC_ != temporaryHeaderCRC) {
              throw(RMAPPacketException("invalid header crc"));
            }
          } else {
            headerCRC_ = temporaryHeaderCRC;
          }
        } else {
          const u32 length_2 = packet[rmapIndex + 8];
          const u32 length_1 = packet[rmapIndex + 9];
          const u32 length_0 = packet[rmapIndex + 10];
          const auto lengthSpecifiedInPacket = (length_2 << 16) + (length_1 << 8) + length_0;
          setDataLength(lengthSpecifiedInPacket);
          const u8 temporaryHeaderCRC = packet[rmapIndex + 11];
          constructHeader();
          if (headerCRCIsChecked_) {
            if (headerCRC_ != temporaryHeaderCRC) {
              throw(RMAPPacketException("invalid header crc"));
            }
          } else {
            headerCRC_ = temporaryHeaderCRC;
          }
          dataIndex = rmapIndex + 12;
          data_.clear();
          for (u32 i = 0; i < lengthSpecifiedInPacket; i++) {
            if ((dataIndex + i) < (length - 1)) {
              data_.push_back(packet[dataIndex + i]);
            } else {
              dataCRC_ = 0x00;  // initialized
              throw(RMAPPacketException("data length mismatch"));
            }
          }

          // length check for DataCRC
          u8 temporaryDataCRC = 0x00;
          if ((dataIndex + lengthSpecifiedInPacket) == (length - 1)) {
            temporaryDataCRC = packet[dataIndex + lengthSpecifiedInPacket];
          } else {
            throw(RMAPPacketException("data length mismatch"));
          }
          dataCRC_ = RMAPUtilities::calculateCRC(data_.data(), data_.size());
          if (dataCRCIsChecked_) {
            if (dataCRC_ != temporaryDataCRC) {
              throw(RMAPPacketException("invalid data crc"));
            }
          } else {
            dataCRC_ = temporaryDataCRC;
          }
        }
      }

    } catch (std::exception& e) {
      throw(RMAPPacketException("packet interpretation failed"));
    }
    const u32 previousHeaderCRCMode = headerCRCMode_;
    const u32 previousDataCRCMode = dataCRCMode_;
    headerCRCMode_ = RMAPPacket::ManualCRC;
    dataCRCMode_ = RMAPPacket::ManualCRC;
    constructPacket();
    headerCRCMode_ = previousHeaderCRCMode;
    dataCRCMode_ = previousDataCRCMode;
  }

  void interpretAsAnRMAPPacket(const std::vector<u8>* data) {
    if (data->empty()) {
      throw(RMAPPacketException("packet interpretation failed"));
    }
    interpretAsAnRMAPPacket(data->data(), data->size());
  }

  void setRMAPTargetInformation(const RMAPTargetNode* rmapTargetNode) {
    setTargetLogicalAddress(rmapTargetNode->getTargetLogicalAddress());
    setReplyAddress(rmapTargetNode->getReplyAddress());
    setTargetSpaceWireAddress(rmapTargetNode->getTargetSpaceWireAddress());
    setKey(rmapTargetNode->getDefaultKey());
    setInitiatorLogicalAddress(rmapTargetNode->getInitiatorLogicalAddress());
  }

  void setCommand() { instruction_ = instruction_ | RMAPPacket::BitMaskForCommandReply; }
  void setReply() { instruction_ = instruction_ & (~RMAPPacket::BitMaskForCommandReply); }
  void setWrite() { instruction_ = instruction_ | RMAPPacket::BitMaskForWriteRead; }
  void setRead() { instruction_ = instruction_ & (~RMAPPacket::BitMaskForWriteRead); }
  void setVerifyFlag(bool mode) {
    if (mode) {
      instruction_ = instruction_ | RMAPPacket::BitMaskForVerifyFlag;
    } else {
      instruction_ = instruction_ & (~RMAPPacket::BitMaskForVerifyFlag);
    }
  }

  void setReplyFlag(bool mode) {
    if (mode) {
      instruction_ = instruction_ | RMAPPacket::BitMaskForReplyFlag;
    } else {
      instruction_ = instruction_ & (~RMAPPacket::BitMaskForReplyFlag);
    }
  }

  void setIncrementFlag(bool mode) {
    if (mode) {
      instruction_ = instruction_ | RMAPPacket::BitMaskForIncrementFlag;
    } else {
      instruction_ = instruction_ & (~RMAPPacket::BitMaskForIncrementFlag);
    }
  }

  void setReplyPathAddressLength(u8 pathAddressLength) {
    // TODO: verify the following logic
    instruction_ = (instruction_ & ~(RMAPPacket::BitMaskForIncrementFlag)) + pathAddressLength &
                  RMAPPacket::BitMaskForIncrementFlag;
  }

  bool isCommand() const { return (instruction_ & RMAPPacket::BitMaskForCommandReply) != 0; }
  bool isReply() const { return (instruction_ & RMAPPacket::BitMaskForCommandReply) == 0; }
  bool isWrite() const { return (instruction_ & RMAPPacket::BitMaskForWriteRead) != 0; }
  bool isRead() const { return (instruction_ & RMAPPacket::BitMaskForWriteRead) == 0; }
  bool isVerifyFlagSet() const { return (instruction_ & RMAPPacket::BitMaskForVerifyFlag) != 0; }
  bool isReplyFlagSet() const { return (instruction_ & RMAPPacket::BitMaskForReplyFlag) != 0; }
  bool isIncrementFlagSet() const { return (instruction_ & RMAPPacket::BitMaskForIncrementFlag) != 0; }

  u8 getReplyPathAddressLength() const { return instruction_ & RMAPPacket::BitMaskForReplyPathAddressLength; }

  u32 getAddress() const { return address_; }

  bool hasData() const { return !data_.empty() || (isCommand() && isWrite()) || (isReply() && isRead()); }

  void getData(u8* buffer, size_t maxLength) {
    const auto length = data_.size();
    if (maxLength < length) {
      throw RMAPPacketException("insufficient buffer size");
    }
    for (size_t i = 0; i < length; i++) {
      buffer[i] = data_[i];
    }
  }

  void getData(std::vector<u8>* buffer) const { *buffer = data_; }

  std::vector<u8>* getDataBuffer() { return &data_; }

  u8 getDataCRC() const { return dataCRC_; }
  u32 getDataLength() const { return dataLength_; }
  u32 getLength() const { return dataLength_; }
  u8 getExtendedAddress() const { return extendedAddress_; }
  u8 getHeaderCRC() const { return headerCRC_; }
  u8 getInitiatorLogicalAddress() const { return initiatorLogicalAddress_; }
  u8 getInstruction() const { return instruction_; }
  u8 getKey() const { return key_; }
  const std::vector<u8>& getReplyAddress() const { return replyAddress; }
  u16 getTransactionID() const { return transactionID_; }
  u8 getStatus() const { return status_; }
  u8 getTargetLogicalAddress() const { return targetLogicalAddress_; }
  const std::vector<u8>& getTargetSpaceWireAddress() const { return targetSpaceWireAddress_; }

  u32 getHeaderCRCMode() const { return headerCRCMode_; }
  u32 getDataCRCMode() const { return dataCRCMode_; }

  void setAddress(u32 address) { this->address_ = address; }

  void setData(const u8* data, size_t length) {
    this->data_.clear();
    for (size_t i = 0; i < length; i++) {
      this->data_.push_back(data[i]);
    }
    this->dataLength_ = length;
  }

  void setDataCRC(u8 dataCRC) { this->dataCRC_ = dataCRC; }
  void setDataLength(u32 dataLength) { this->dataLength_ = dataLength; }
  void setLength(u32 dataLength) { this->dataLength_ = dataLength; }
  void setExtendedAddress(u8 extendedAddress) { this->extendedAddress_ = extendedAddress; }
  void setHeaderCRC(u8 headerCRC) { this->headerCRC_ = headerCRC; }
  void setInitiatorLogicalAddress(u8 initiatorLogicalAddress) {
    this->initiatorLogicalAddress_ = initiatorLogicalAddress;
  }
  void setInstruction(u8 instruction) { this->instruction_ = instruction; }
  void setKey(u8 key) { this->key_ = key; }
  void setReplyAddress(const std::vector<u8>& replyAddress,  //
                       bool automaticallySetPathAddressLengthToInstructionField = true) {
    this->replyAddress = replyAddress;
    if (automaticallySetPathAddressLengthToInstructionField) {
      if (replyAddress.size() % 4 == 0) {
        instruction_ = (instruction_ & (~BitMaskForReplyPathAddressLength)) + replyAddress.size() / 4;
      } else {
        instruction_ = (instruction_ & (~BitMaskForReplyPathAddressLength)) + (replyAddress.size() + 4) / 4;
      }
    }
  }

  void setTransactionID(u16 transactionID) { this->transactionID_ = transactionID; }
  void setStatus(u8 status) { this->status_ = status; }
  void setHeaderCRCMode(u32 headerCRCMode) { this->headerCRCMode_ = headerCRCMode; }
  void setDataCRCMode(u32 dataCRCMode) { this->dataCRCMode_ = dataCRCMode; }

  void addData(u8 oneByte) { this->data_.push_back(oneByte); }
  void clearData() { data_.clear(); }
  void addData(const std::vector<u8>& array) { data_.insert(data_.end(), array.begin(), array.end()); }

  std::string toString() { return isCommand() ? toStringCommandPacket() : toStringReplyPacket(); }

  void setTargetLogicalAddress(u8 targetLogicalAddress) { this->targetLogicalAddress_ = targetLogicalAddress; }
  void setTargetSpaceWireAddress(const std::vector<u8>& targetSpaceWireAddress) {
    this->targetSpaceWireAddress_ = targetSpaceWireAddress;
  }

  std::vector<u8>* getPacketBufferPointer() {
    constructPacket();
    return &wholePacket_;
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

  bool getDataCRCIsChecked() const { return dataCRCIsChecked_; }
  bool getHeaderCRCIsChecked() const { return headerCRCIsChecked_; }
  void setDataCRCIsChecked(bool dataCRCIsChecked) { this->dataCRCIsChecked_ = dataCRCIsChecked; }
  void setHeaderCRCIsChecked(bool headerCRCIsChecked) { this->headerCRCIsChecked_ = headerCRCIsChecked; }

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
      if (!nonZeroValueHasAppeared && replyAddress[i] != 0x00) {
        nonZeroValueHasAppeared = true;
      }
      if (nonZeroValueHasAppeared) {
        result.push_back(replyAddress[i]);
      }
    }
    return result;
  }

  std::string toStringCommandPacket() {
    using namespace std;

    stringstream ss;
    ///////////////////////////////
    // Command
    ///////////////////////////////
    // Target SpaceWire Address
    if (!targetSpaceWireAddress_.empty()) {
      ss << "--------- Target SpaceWire Address ---------" << endl;
      spacewire::util::dumpPacket(&ss, targetSpaceWireAddress_.data(), targetSpaceWireAddress_.size(), 1, 128);
    }
    // Header
    ss << "--------- RMAP Header Part ---------" << endl;
    // Initiator Logical Address
    ss << "Initiator Logical Address : 0x" << right << setw(2) << setfill('0') << hex << (u32)(initiatorLogicalAddress_)
       << endl;
    // Target Logical Address
    ss << "Target Logic. Address     : 0x" << right << setw(2) << setfill('0') << hex
       << (unsigned int)(targetLogicalAddress_) << endl;
    // Protocol Identifier
    ss << "Protocol ID               : 0x" << right << setw(2) << setfill('0') << hex << (unsigned int)(1) << endl;
    // Instruction
    ss << "Instruction               : 0x" << right << setw(2) << setfill('0') << hex << (unsigned int)(instruction_)
       << endl;
    toStringInstructionField(ss);
    // Key
    ss << "Key                       : 0x" << setw(2) << setfill('0') << hex << (unsigned int)(key_) << endl;
    // Reply Address
    if (!replyAddress.empty()) {
      ss << "Reply Address             : ";
      spacewire::util::dumpPacket(&ss, replyAddress.data(), replyAddress.size(), 1, 128);
    } else {
      ss << "Reply Address         : --none--" << endl;
    }
    ss << "Transaction Identifier    : 0x" << right << setw(4) << setfill('0') << hex << (unsigned int)(transactionID_)
       << endl;
    ss << "Extended Address          : 0x" << right << setw(2) << setfill('0') << hex << (unsigned int)(extendedAddress_)
       << endl;
    ss << "Address                   : 0x" << right << setw(8) << setfill('0') << hex << (unsigned int)(address_)
       << endl;
    ss << "Data Length (bytes)       : 0x" << right << setw(6) << setfill('0') << hex << (unsigned int)(dataLength_)
       << " (" << dec << dataLength_ << "dec)" << endl;
    ss << "Header CRC                : 0x" << right << setw(2) << setfill('0') << hex << (unsigned int)(headerCRC_)
       << endl;
    // Data Part
    ss << "---------  RMAP Data Part  ---------" << endl;
    if (isWrite()) {
      ss << "[data size = " << dec << dataLength_ << "bytes]" << endl;
      spacewire::util::dumpPacket(&ss, data_.data(), data_.size(), 1, 16);
      ss << "Data CRC                  : " << right << setw(2) << setfill('0') << hex << (unsigned int)(dataCRC_)
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
    if (!replyAddress.empty()) {
      ss << "--------- Reply Address ---------" << endl;
      ss << "Reply Address       : ";
      spacewire::util::dumpPacket(&ss, replyAddress.data(), replyAddress.size(), 1, 128);
    }
    // Header
    ss << "--------- RMAP Header Part ---------" << endl;
    // Initiator Logical Address
    ss << "Initiator Logical Address : 0x" << right << setw(2) << setfill('0') << hex << (u32)(initiatorLogicalAddress_)
       << endl;
    // Target Logical Address
    ss << "Target Logical Address    : 0x" << right << setw(2) << setfill('0') << hex
       << (unsigned int)(targetLogicalAddress_) << endl;
    // Protocol Identifier
    ss << "Protocol ID               : 0x" << right << setw(2) << setfill('0') << hex << (unsigned int)(1) << endl;
    // Instruction
    ss << "Instruction               : 0x" << right << setw(2) << setfill('0') << hex << (unsigned int)(instruction_)
       << endl;
    toStringInstructionField(ss);
    // Status
    std::string statusstring;
    switch (status_) {
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
    ss << "Status                    : 0x" << right << setw(2) << setfill('0') << hex << (unsigned int)(status_) << " ("
       << statusstring << ")" << endl;
    ss << "Transaction Identifier    : 0x" << right << setw(4) << setfill('0') << hex << (unsigned int)(transactionID_)
       << endl;
    if (isRead()) {
      ss << "Data Length (bytes)       : 0x" << right << setw(6) << setfill('0') << hex << (unsigned int)(dataLength_)
         << " (" << dec << dataLength_ << "dec)" << endl;
    }
    ss << "Header CRC                : 0x" << right << setw(2) << setfill('0') << hex << (unsigned int)(headerCRC_)
       << endl;
    // Data Part
    ss << "---------  RMAP Data Part  ---------" << endl;
    if (isRead()) {
      ss << "[data size = " << dec << dataLength_ << "bytes]" << endl;
      spacewire::util::dumpPacket(&ss, data_.data(), data_.size(), 1, 128);
      ss << "Data CRC    : 0x" << right << setw(2) << setfill('0') << hex << (unsigned int)(dataCRC_) << endl;
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
       << (u32)((instruction_ & BitMaskForReplyPathAddressLength)) << endl;
    ss << " |(R.A.L. = Reply Address Length)" << endl;
    ss << " ------------------------------" << endl;
  }

  std::vector<u8> targetSpaceWireAddress_{};
  u8 targetLogicalAddress_ = SpaceWireProtocol::DEFAULT_LOGICAL_ADDRESS;

  u8 instruction_{};
  u8 key_ = RMAPProtocol::DEFAULT_KEY;
  std::vector<u8> replyAddress{};
  u8 initiatorLogicalAddress_ = SpaceWireProtocol::DEFAULT_LOGICAL_ADDRESS;
  u8 extendedAddress_ = RMAPProtocol::DEFAULT_EXTENDED_ADDRESS;
  u32 address_{};
  u32 dataLength_{};
  u8 status_ = RMAPProtocol::DEFAULT_STATUS;
  u8 headerCRC_{};
  u16 transactionID_ = RMAPProtocol::DEFAULT_TID;

  std::vector<u8> header_{};
  std::vector<u8> data_{};
  u8 dataCRC_{};

  std::vector<u8> wholePacket_{};

  u32 headerCRCMode_ = RMAPPacket::AutoCRC;
  u32 dataCRCMode_ = RMAPPacket::AutoCRC;

  std::vector<u8> temporaryPathAddress_{};

  bool headerCRCIsChecked_{};
  bool dataCRCIsChecked_{};
};
#endif
