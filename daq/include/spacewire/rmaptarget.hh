#ifndef SPACEWIRE_RMAPTARGET_HH_
#define SPACEWIRE_RMAPTARGET_HH_

#include "RMAPTransaction.hh"

class RMAPAddressRange {
public:
	u32 addressFrom;
	u32 addressTo;

public:
	u32 getAddressFrom() const {
		return addressFrom;
	}

	u32 getAddressTo() const {
		return addressTo;
	}

	void setAddressFrom(u32 addressFrom) {
		this->addressFrom = addressFrom;
	}

	void setAddressTo(u32 addressTo) {
		this->addressTo = addressTo;
	}

public:
	void setByLength(u32 addressFrom, u32 lengthInBytes) {
		this->addressFrom = addressFrom;
		this->addressTo = addressFrom + lengthInBytes;
	}

public:
	RMAPAddressRange(u32 addressFrom, u32 addressTo) :
			addressFrom(addressFrom), addressTo(addressTo) {

	}

	RMAPAddressRange() {
		addressFrom = 0;
		addressTo = 0;
	}

public:
	bool contains(RMAPAddressRange& addressRange) {
		if (addressRange.addressTo < addressFrom || addressTo < addressRange.addressFrom) {
			return false;
		}
		if (addressFrom <= addressRange.addressFrom && addressRange.addressTo <= addressTo) {
			return true;
		} else {
			return false;
		}
	}
};

class RMAPTargetAccessActionException: public CxxUtilities::Exception {
public:
	enum {
		AccessToUndefinedAddressRange
	};

public:
	RMAPTargetAccessActionException(int status) :
			CxxUtilities::Exception(status) {

	}

	virtual ~RMAPTargetAccessActionException() {
	}
};

class RMAPTargetAccessAction {
public:
	virtual ~RMAPTargetAccessAction() {
	}

public:
	virtual void processTransaction(RMAPTransaction* rmapTransaction) throw (RMAPTargetAccessActionException)= 0;

	virtual void transactionWillComplete(RMAPTransaction* rmapTransaction) throw (RMAPTargetAccessActionException) {
		delete rmapTransaction->replyPacket;
	}

	virtual void transactionReplyCouldNotBeSent(RMAPTransaction* rmapTransaction)
			throw (RMAPTargetAccessActionException) {
		delete rmapTransaction->replyPacket;
	}

public:
	void setReplyWithDataWithStatus(RMAPTransaction* rmapTransaction, std::vector<u8>* data, u8 status) {
		rmapTransaction->replyPacket = RMAPPacket::constructReplyForCommand(rmapTransaction->commandPacket, status);
		rmapTransaction->replyPacket->setData(*data);
	}

	void setReplyWithStatus(RMAPTransaction* rmapTransaction, u8 status) {
		rmapTransaction->replyPacket = RMAPPacket::constructReplyForCommand(rmapTransaction->commandPacket, status);
		rmapTransaction->replyPacket->clearData();
	}

};

class RMAPTargetException: public CxxUtilities::Exception {
public:
	enum {
		AccessToUndefinedAddressRange
	};

public:
	RMAPTargetException(int status) :
			CxxUtilities::Exception(status) {
	}

	virtual ~RMAPTargetException() {
	}

};

class RMAPTarget {
private:
	std::map<RMAPAddressRange*, RMAPTargetAccessAction*> actions;
	std::vector<RMAPAddressRange*> addressRanges;

public:
	RMAPTarget() {

	}

	~RMAPTarget() {

	}

public:
	void addAddressRangeAndAssociatedAction(RMAPAddressRange* addressRange, RMAPTargetAccessAction* action) {
		actions[addressRange] = action;
		addressRanges.push_back(addressRange);
	}

	bool doesAcceptAddressRange(RMAPAddressRange addressRange) {
		for (size_t i = 0; i < addressRanges.size(); i++) {
			if (addressRanges[i]->contains(addressRange) == true) {
				return true;
			}
		}
		return false;
	}

	bool doesAcceptTransaction(RMAPTransaction* rmapTransaction) {
		u32 addressFrom = rmapTransaction->commandPacket->getAddress();
		u32 addressTo = addressFrom + rmapTransaction->commandPacket->getLength();
		RMAPAddressRange addressRange(addressFrom, addressTo);
		for (size_t i = 0; i < addressRanges.size(); i++) {
			if (addressRanges[i]->contains(addressRange) == true) {
				return true;
			}
		}
		return false;
	}

	void processTransaction(RMAPTransaction* rmapTransaction) throw (RMAPTargetException) {
		u32 addressFrom = rmapTransaction->commandPacket->getAddress();
		u32 addressTo = addressFrom + rmapTransaction->commandPacket->getLength();
		RMAPAddressRange addressRange(addressFrom, addressTo);
		for (size_t i = 0; i < addressRanges.size(); i++) {
			if (addressRanges[i]->contains(addressRange) == true) {
				try {
					actions[addressRanges[i]]->processTransaction(rmapTransaction);
					return;
				} catch (...) {
					throw RMAPTargetException(RMAPTargetException::AccessToUndefinedAddressRange);
				}
			}
		}
		throw RMAPTargetException(RMAPTargetException::AccessToUndefinedAddressRange);
	}

	/** Can return NULL. */
	RMAPTargetAccessAction* getCorrespondingRMAPTargetAccessAction(RMAPTransaction* rmapTransaction) {
		using namespace std;
		u32 addressFrom = rmapTransaction->commandPacket->getAddress();
		u32 addressTo = addressFrom + rmapTransaction->commandPacket->getLength() - 1;
		RMAPAddressRange addressRange(addressFrom, addressTo);
		for (size_t i = 0; i < addressRanges.size(); i++) {
			if (addressRanges[i]->contains(addressRange) == true) {
				return actions[addressRanges[i]];
			}
		}
		return NULL;
	}

};

#endif
