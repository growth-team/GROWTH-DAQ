#ifndef SPACEWIRE_SPACEWIREPACKET_HH_
#define SPACEWIRE_SPACEWIREPACKET_HH_

#include <vector>

#include "spacewire/spacewireprotocol.hh"
#include "spacewire/types.hh"

class SpaceWirePacket {
 public:
  SpaceWirePacket() = default;
  SpaceWirePacket(u8 protocolID) : protocolID_(protocolID) {}
  virtual ~SpaceWirePacket() = default;
  void setProtocolID(u8 protocolID) { protocolID_ = protocolID; }
  u8 getProtocolID() const { return protocolID_; }
  EOPType getEOPType() { return eopType_; }
  void setEOPType(EOPType eopType) { eopType_ = eopType; }

  std::vector<u8> data{};

 protected:
  u8 protocolID_ = SpaceWireProtocol::DefaultProtocolID;
  EOPType eopType_ = EOPType::EOP;
};

#endif
