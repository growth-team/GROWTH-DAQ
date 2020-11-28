#ifndef SPACEWIRE_RMAPTRANSACTION_HH_
#define SPACEWIRE_RMAPTRANSACTION_HH_

#include "spacewire/types.hh"
#include "rmappacket.hh"

#include "CxxUtilities/CxxUtilities.hh"

#include <mutex>

class RMAPTransaction {
public:
	static constexpr f64 DefaultTimeoutDuration = 1000.0;

	u8 targetLogicalAddress{};
	u8 initiatorLogicalAddress{};
	u16 transactionID{};
	CxxUtilities::Condition condition;
	double timeoutDuration  = DefaultTimeoutDuration;
	u32 state{};
	std::mutex stateMutex;
	RMAPPacket* commandPacket{};
	RMAPPacket* replyPacket{};

	enum {
		//for RMAPInitiator-related transaction
		NotInitiated = 0x00,
		Initiated = 0x01,
		CommandSent = 0x02,
		ReplyReceived = 0x03,
		Timeout = 0x04,
		//for RMAPTarget-related transaction
		CommandPacketReceived = 0x10,
		ReplySet = 0x11,
		ReplySent = 0x12,
		Aborted = 0x13,
		ReplyCompleted = 0x14
	};
};

#endif
