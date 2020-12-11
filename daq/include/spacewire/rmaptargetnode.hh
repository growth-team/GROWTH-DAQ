#ifndef SPACEWIRE_RMAPTARGETNODE_HH_
#define SPACEWIRE_RMAPTARGETNODE_HH_

#include "spacewire/types.hh"
#include "spacewire/spacewireutilities.hh"

class RMAPTargetNode {
 public:
  RMAPTargetNode() = default;

  u8 getDefaultKey() const { return defaultKey; }
  const std::vector<u8>& getReplyAddress() const { return replyAddress; }
  u8 getTargetLogicalAddress() const { return targetLogicalAddress; }
  const std::vector<u8>& getTargetSpaceWireAddress() const { return targetSpaceWireAddress; }
  u8 getInitiatorLogicalAddress() const { return initiatorLogicalAddress; }

  void setDefaultKey(u8 defaultKey) { this->defaultKey = defaultKey; }
  void setReplyAddress(const std::vector<u8>& replyAddress) { this->replyAddress = replyAddress; }
  void setTargetLogicalAddress(u8 targetLogicalAddress) { this->targetLogicalAddress = targetLogicalAddress; }
  void setTargetSpaceWireAddress(const std::vector<u8>& spaceWireAddress) { targetSpaceWireAddress = spaceWireAddress; }
  void setInitiatorLogicalAddress(u8 logicalAddress) { initiatorLogicalAddress = logicalAddress; }

  std::string toString(size_t nTabs = 0) const {
    using namespace std;
    stringstream ss;
    ss << "Initiator Logical Address : 0x" << right << hex << setw(2) << setfill('0')
       << (uint32_t)initiatorLogicalAddress << endl;
    ss << "Target Logical Address    : 0x" << right << hex << setw(2) << setfill('0')
       << static_cast<uint32_t>(targetLogicalAddress) << endl;
    ss << "Target SpaceWire Address  : "
       << spacewire::util::packetToString(targetSpaceWireAddress.data(), targetSpaceWireAddress.size()) << '\n';
    ss << "Reply Address             : " << spacewire::util::packetToString(replyAddress.data(), replyAddress.size())
       << '\n';
    ss << "Default Key               : 0x" << right << hex << setw(2) << setfill('0')
       << static_cast<uint32_t>(defaultKey) << endl;
    stringstream ss2;
    while (!ss.eof()) {
      std::string line{};
      getline(ss, line);
      for (size_t i = 0; i < nTabs; i++) {
        ss2 << "	";
      }
      ss2 << line << endl;
    }
    return ss2.str();
  }

  static const u8 DefaultLogicalAddress = 0xFE;
  static const u8 DefaultKey = 0x20;

 private:
  std::vector<u8> targetSpaceWireAddress{};
  std::vector<u8> replyAddress{};
  u8 targetLogicalAddress = DefaultLogicalAddress;
  u8 initiatorLogicalAddress = DefaultLogicalAddress;
  u8 defaultKey = DefaultKey;
};
#endif
