#ifndef SPACEWIRE_SPACEWIREPACKET_HH_
#define SPACEWIRE_SPACEWIREPACKET_HH_

#include <vector>

#include "spacewire/types.hh"

class SpaceWirePacket {
 public:
  SpaceWirePacket() = default;
  virtual ~SpaceWirePacket() = default;
  void setProtocolID(u8 protocolID) { protocolID_ = protocolID; }
  u8 getProtocolID() const { return protocolID_; }
  EOPType getEOPType() { return eopType_; }
  void setEOPType(EOPType eopType) { eopType_ = eopType; }

  static const u8 DefaultLogicalAddress = 0xFE;
  static const u8 DefaultProtocolID = 0x00;
  std::vector<u8> data{};

 protected:
  u8 protocolID_ = DefaultProtocolID;
  EOPType eopType_ = EOPType::EOP;
};

#endif
