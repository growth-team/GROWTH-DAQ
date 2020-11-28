#ifndef SPACEWIRE_RMAPPROTOCOL_HH_
#define SPACEWIRE_RMAPPROTOCOL_HH_

class RMAPProtocol {
public:
	static const u8 ProtocolIdentifier = 0x01;
	static const u8 DefaultPacketType = 0x01;
	static const u8 DefaultWriteOrRead = 0x01;
	static const u8 DefaultVerifyMode = 0x01;
	static const u8 DefaultAckMode = 0x01;
	static const u8 DefaultIncrementMode = 0x01;
	static const u16 DefaultTID = 0x00;
	static const u8 DefaultExtendedAddress = 0x00;
	static const u32 DefaultAddress = 0x00;
	static const u32 DefaultLength = 0x00;
	static const u8 DefaultHeaderCRC = 0x00;
	static const u8 DefaultDataCRC = 0x00;
	static const u8 DefaultStatus = 0x00;

	static const u8 PacketTypeCommand = 0x01;
	static const u8 PacketTypeReply = 0x00;

	static const u8 PacketWriteMode = 0x01;
	static const u8 PacketReadMode = 0x00;

	static const u8 PacketVerifyMode = 0x01;
	static const u8 PacketNoVerifyMode = 0x00;

	static const u8 PacketAckMode = 0x01;
	static const u8 PacketNoAckMode = 0x00;

	static const u8 PacketIncrementMode = 0x01;
	static const u8 PacketNoIncrementMode = 0x00;

	static const u8 PacketCRCDraftE = 0x00;
	static const u8 PacketCRCDraftF = 0x01;

	static const u8 DefaultKey = 0x20;

	static const bool DefaultCRCCheckMode = true;

	static const u8 DefaultLogicalAddress = 0xFE;

};

#endif
