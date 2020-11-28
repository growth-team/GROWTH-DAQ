#ifndef SPACEWIRE_RMAPPACKET_HH_
#define SPACEWIRE_RMAPPACKET_HH_

#include <CxxUtilities/CommonHeader.hh>

#include "SpaceWirePacket.hh"
#include "SpaceWireUtilities.hh"
#include "SpaceWireProtocol.hh"

#include "RMAPProtocol.hh"
#include "RMAPUtilities.hh"
#include "RMAPTargetNode.hh"
#include "RMAPReplyStatus.hh"

class RMAPPacketException: public CxxUtilities::Exception {
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
	RMAPPacketException(uint32_t status) :
			CxxUtilities::Exception(status) {
	}

public:
	virtual ~RMAPPacketException() {
	}

public:
	virtual std::string toString() {
		//return ClassInformation::demangle(typeid(*this).name());
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

class RMAPPacket: public SpaceWirePacket {
private:
	std::vector<u8> targetSpaceWireAddress;
	u8 targetLogicalAddress;

private:
	u8 instruction;
	u8 key;
	std::vector<u8> replyAddress;
	u8 initiatorLogicalAddress;
	u8 extendedAddress;
	uint32_t address;
	uint32_t dataLength;
	u8 status;
	u8 headerCRC;

private:
	uint16_t transactionID;

private:
	std::vector<u8> header;

private:
	std::vector<u8> data;
	u8 dataCRC;

private:
	uint32_t headerCRCMode;
	uint32_t dataCRCMode;

public:
	bool headerCRCIsChecked;
	bool dataCRCIsChecked;

private:
	bool useDraftECRC;

public:
	enum {
		AutoCRC = 0x00, ManualCRC = 0x01
	};

public:
	static const u8 BitMaskForReserved = 0x80;
	static const u8 BitMaskForCommandReply = 0x40;
	static const u8 BitMaskForWriteRead = 0x20;
	static const u8 BitMaskForVerifyFlag = 0x10;
	static const u8 BitMaskForReplyFlag = 0x08;
	static const u8 BitMaskForIncrementFlag = 0x04;
	static const u8 BitMaskForReplyPathAddressLength = 0x3;

public:
	bool isUseDraftECRC() const {
		return useDraftECRC;
	}

public:
	void setUseDraftECRC(bool useDraftEcrc) {
		this->useDraftECRC = useDraftEcrc;
	}

public:
	bool getDataCRCIsChecked() const {
		return dataCRCIsChecked;
	}

public:
	bool getHeaderCRCIsChecked() const {
		return headerCRCIsChecked;
	}

public:
	void setDataCRCIsChecked(bool dataCRCIsChecked) {
		this->dataCRCIsChecked = dataCRCIsChecked;
	}

public:
	void setHeaderCRCIsChecked(bool headerCRCIsChecked) {
		this->headerCRCIsChecked = headerCRCIsChecked;
	}

public:
	RMAPPacket() {
		initiatorLogicalAddress = SpaceWireProtocol::DefaultLogicalAddress;
		protocolID_ = RMAPProtocol::ProtocolIdentifier;
		targetLogicalAddress = SpaceWireProtocol::DefaultLogicalAddress;
		key = RMAPProtocol::DefaultKey;
		transactionID = RMAPProtocol::DefaultTID;
		status = RMAPProtocol::DefaultStatus;
		instruction = 0x00;
		headerCRCMode = RMAPPacket::AutoCRC;
		dataCRCMode = RMAPPacket::AutoCRC;
		headerCRCIsChecked = RMAPProtocol::DefaultCRCCheckMode;
		dataCRCIsChecked = RMAPProtocol::DefaultCRCCheckMode;
		useDraftECRC = false;
		dataLength = 0;
		address = 0x00;
		extendedAddress = RMAPProtocol::DefaultExtendedAddress;
		headerCRC = 0;
		dataCRC = 0;
	}

public:
	void constructHeader() {
		using namespace std;

		header.clear();
		if (isCommand()) {
			//if command packet
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
			header.push_back((u8) ((((((transactionID & 0xff00) >> 8))))));
			header.push_back((u8) ((((((transactionID & 0x00ff) >> 0))))));
			header.push_back(extendedAddress);
			header.push_back((u8) ((((((address & 0xff000000) >> 24))))));
			header.push_back((u8) ((((((address & 0x00ff0000) >> 16))))));
			header.push_back((u8) ((((((address & 0x0000ff00) >> 8))))));
			header.push_back((u8) ((((((address & 0x000000ff) >> 0))))));
			header.push_back((u8) ((((((dataLength & 0x00ff0000) >> 16))))));
			header.push_back((u8) ((((((dataLength & 0x0000ff00) >> 8))))));
			header.push_back((u8) ((((((dataLength & 0x000000ff) >> 0))))));
		} else {
			//if reply packet
			header.push_back(initiatorLogicalAddress);
			header.push_back(protocolID_);
			header.push_back(instruction);
			header.push_back(status);
			header.push_back(targetLogicalAddress);
			header.push_back((u8) ((((((transactionID & 0xff00) >> 8))))));
			header.push_back((u8) ((((((transactionID & 0x00ff) >> 0))))));
			if (isRead()) {
				header.push_back(0);
				header.push_back((u8) ((((((dataLength & 0x00ff0000) >> 16))))));
				header.push_back((u8) ((((((dataLength & 0x0000ff00) >> 8))))));
				header.push_back((u8) ((((((dataLength & 0x000000ff) >> 0))))));
			}
		}

		if (headerCRCMode == RMAPPacket::AutoCRC) {
			calculateHeaderCRC();
		}
		header.push_back(headerCRC);
	}

public:
	inline void calculateHeaderCRC() {
		if (!useDraftECRC) {
			headerCRC = RMAPUtilities::calculateCRC(header);
		} else {
			headerCRC = RMAPUtilities::calculateCRCBasedOnDraftESpecification(header);
		}
	}

public:
	inline void calculateDataCRC() {
		if (!useDraftECRC) {
			dataCRC = RMAPUtilities::calculateCRC(data);
		} else {
			dataCRC = RMAPUtilities::calculateCRCBasedOnDraftESpecification(data);
		}
	}

public:
	void constructPacket() {
		using namespace std;

		constructHeader();
		if (dataCRCMode == RMAPPacket::AutoCRC) {
			calculateDataCRC();
		}
		wholePacket.clear();
		if (isCommand() == true) {
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

public:
	void interpretAsAnRMAPPacket(u8 *packet, size_t length) throw (RMAPPacketException) {
		using namespace std;

		if (length < 8) {
			throw(RMAPPacketException(RMAPPacketException::PacketInterpretationFailed));
		}
		std::vector<u8> temporaryPathAddress;
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
				//if command packet
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
				setTransactionID((uint32_t) (((uppertid * 0x100 + lowertid))));
				setExtendedAddress(packet[rmapIndexAfterSourcePathAddress + 3]);
				u8 address_3, address_2, address_1, address_0;
				address_3 = packet[rmapIndexAfterSourcePathAddress + 4];
				address_2 = packet[rmapIndexAfterSourcePathAddress + 5];
				address_1 = packet[rmapIndexAfterSourcePathAddress + 6];
				address_0 = packet[rmapIndexAfterSourcePathAddress + 7];
				setAddress(address_3 * 0x01000000 + address_2 * 0x00010000 + address_1 * 0x00000100 + address_0 * 0x00000001);
				u8 length_2, length_1, length_0;
				uint32_t lengthSpecifiedInPacket;
				length_2 = packet[rmapIndexAfterSourcePathAddress + 8];
				length_1 = packet[rmapIndexAfterSourcePathAddress + 9];
				length_0 = packet[rmapIndexAfterSourcePathAddress + 10];
				lengthSpecifiedInPacket = length_2 * 0x010000 + length_1 * 0x000100 + length_0 * 0x000001;
				setDataLength(lengthSpecifiedInPacket);
				u8 temporaryHeaderCRC = packet[rmapIndexAfterSourcePathAddress + 11];
				if (headerCRCIsChecked == true) {
					uint32_t headerCRCMode_original = headerCRCMode;
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
					for (uint32_t i = 0; i < lengthSpecifiedInPacket; i++) {
						if ((dataIndex + i) < (length - 1)) {
							data.push_back(packet[dataIndex + i]);
						} else {
							throw(RMAPPacketException(RMAPPacketException::DataLengthMismatch));
						}
					}

					//length check for DataCRC
					u8 temporaryDataCRC = 0x00;
					if ((dataIndex + lengthSpecifiedInPacket) == (length - 1)) {
						temporaryDataCRC = packet[dataIndex + lengthSpecifiedInPacket];
					} else {
						throw(RMAPPacketException(RMAPPacketException::DataLengthMismatch));
					}
					calculateDataCRC();
					if (dataCRCIsChecked == true) {
						if (dataCRC != temporaryDataCRC) {
							throw(RMAPPacketException(RMAPPacketException::InvalidDataCRC));
						}
					} else {
						dataCRC = temporaryDataCRC;
					}
				}

			} else {
				//if reply packet
				setReplyAddress(temporaryPathAddress, false);
				setInitiatorLogicalAddress(packet[rmapIndex]);
				setStatus(packet[rmapIndex + 3]);
				setTargetLogicalAddress(packet[rmapIndex + 4]);
				u8 uppertid, lowertid;
				uppertid = packet[rmapIndex + 5];
				lowertid = packet[rmapIndex + 6];
				setTransactionID((uint32_t) (((uppertid * 0x100 + lowertid))));
				if (isWrite()) {
					u8 temporaryHeaderCRC = packet[rmapIndex + 7];
					uint32_t headerCRCMode_original = headerCRCMode;
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
					uint32_t lengthSpecifiedInPacket;
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
					for (uint32_t i = 0; i < lengthSpecifiedInPacket; i++) {
						if ((dataIndex + i) < (length - 1)) {
							data.push_back(packet[dataIndex + i]);
						} else {
							dataCRC = 0x00; //initialized
							throw(RMAPPacketException(RMAPPacketException::DataLengthMismatch));
						}
					}

					//length check for DataCRC
					u8 temporaryDataCRC = 0x00;
					if ((dataIndex + lengthSpecifiedInPacket) == (length - 1)) {
						temporaryDataCRC = packet[dataIndex + lengthSpecifiedInPacket];
					} else {
						throw(RMAPPacketException(RMAPPacketException::DataLengthMismatch));
					}
					calculateDataCRC();
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
		uint32_t previousHeaderCRCMode = headerCRCMode;
		uint32_t previousDataCRCMode = dataCRCMode;
		headerCRCMode = RMAPPacket::ManualCRC;
		dataCRCMode = RMAPPacket::ManualCRC;
		constructPacket();
		headerCRCMode = previousHeaderCRCMode;
		dataCRCMode = previousDataCRCMode;
	}

public:
	void interpretAsAnRMAPPacket(std::vector<u8> & data) throw (RMAPPacketException) {
		if (data.size() == 0) {
			throw RMAPPacketException(RMAPPacketException::PacketInterpretationFailed);
		}
		interpretAsAnRMAPPacket(&(data[0]), data.size());
	}

public:
	void interpretAsAnRMAPPacket(std::vector<u8> *data) throw (RMAPPacketException) {
		if (data->size() == 0) {
			throw RMAPPacketException(RMAPPacketException::PacketInterpretationFailed);
		}
		interpretAsAnRMAPPacket(&(data->at(0)), data->size());
	}

public:
	void setRMAPTargetInformation(RMAPTargetNode *rmapTargetNode) {
		setTargetLogicalAddress(rmapTargetNode->getTargetLogicalAddress());
		setReplyAddress(rmapTargetNode->getReplyAddress());
		setTargetSpaceWireAddress(rmapTargetNode->getTargetSpaceWireAddress());
		setKey(rmapTargetNode->getDefaultKey());
		if (rmapTargetNode->isInitiatorLogicalAddressSet()) {
			setInitiatorLogicalAddress(rmapTargetNode->getInitiatorLogicalAddress());
		}
	}

public:
	inline void setRMAPTargetInformation(RMAPTargetNode & rmapTargetNode) {
		setRMAPTargetInformation(&rmapTargetNode);
	}

public:
	inline bool isCommand() {
		if ((instruction & RMAPPacket::BitMaskForCommandReply) > 0) {
			return true;
		} else {
			return false;
		}
	}

public:
	inline void setCommand() {
		instruction = instruction | RMAPPacket::BitMaskForCommandReply;
	}

public:
	inline bool isReply() {
		if ((instruction & RMAPPacket::BitMaskForCommandReply) > 0) {
			return false;
		} else {
			return true;
		}
	}

public:
	inline void setReply() {
		instruction = instruction & (~RMAPPacket::BitMaskForCommandReply);
	}

public:
	inline bool isWrite() {
		if ((instruction & RMAPPacket::BitMaskForWriteRead) > 0) {
			return true;
		} else {
			return false;
		}
	}

public:
	inline void setWrite() {
		instruction = instruction | RMAPPacket::BitMaskForWriteRead;
	}

public:
	inline bool isRead() {
		if ((instruction & RMAPPacket::BitMaskForWriteRead) > 0) {
			return false;
		} else {
			return true;
		}
	}

public:
	inline void setRead() {
		instruction = instruction & (~RMAPPacket::BitMaskForWriteRead);
	}

public:
	inline bool isVerifyFlagSet() {
		if ((instruction & RMAPPacket::BitMaskForVerifyFlag) > 0) {
			return true;
		} else {
			return false;
		}
	}

public:
	inline void setVerifyFlag() {
		instruction = instruction | RMAPPacket::BitMaskForVerifyFlag;
	}

public:
	inline void unsetVerifyFlag() {
		instruction = instruction & (~RMAPPacket::BitMaskForVerifyFlag);
	}

public:
	inline void setVerifyMode() {
		setVerifyFlag();
	}

public:
	inline void setNoVerifyMode() {
		unsetVerifyFlag();
	}

public:
	inline bool isReplyFlagSet() {
		if ((instruction & RMAPPacket::BitMaskForReplyFlag) > 0) {
			return true;
		} else {
			return false;
		}
	}

public:
	inline void setReplyFlag() {
		instruction = instruction | RMAPPacket::BitMaskForReplyFlag;
	}

public:
	inline void unsetReplyFlag() {
		instruction = instruction & (~RMAPPacket::BitMaskForReplyFlag);
	}

public:
	inline void setReplyMode() {
		setReplyFlag();
	}

public:
	inline void setNoReplyMode() {
		unsetReplyFlag();
	}

public:
	inline bool isIncrementFlagSet() {
		if ((instruction & RMAPPacket::BitMaskForIncrementFlag) > 0) {
			return true;
		} else {
			return false;
		}
	}

public:
	inline void setIncrementFlag() {
		instruction = instruction | RMAPPacket::BitMaskForIncrementFlag;
	}

public:
	inline void unsetIncrementFlag() {
		instruction = instruction & (~RMAPPacket::BitMaskForIncrementFlag);
	}

public:
	inline void setIncrementMode() {
		setIncrementFlag();
	}

public:
	inline void setNoIncrementMode() {
		unsetIncrementFlag();
	}

public:
	inline u8 getReplyPathAddressLength() {
		return instruction & RMAPPacket::BitMaskForReplyPathAddressLength;
	}

public:
	inline void setReplyPathAddressLength(u8 pathAddressLength) {
		instruction = (instruction & ~(RMAPPacket::BitMaskForIncrementFlag)) + pathAddressLength
				& RMAPPacket::BitMaskForIncrementFlag;
	}

public:
	uint32_t getAddress() const {
		return address;
	}

public:
	bool hasData() {
		if (data.size() != 0) {
			return true;
		} else {
			if ((isCommand() && isWrite()) || (isReply() && isRead())) {
				return true;
			} else {
				return false;
			}
		}

	}

public:
	std::vector<u8> getData() const {
		return data;
	}

public:
	void getData(u8 *buffer, size_t maxLength) throw (RMAPPacketException) {
		size_t length = data.size();
		if (maxLength < length) {
			throw RMAPPacketException(RMAPPacketException::InsufficientBufferSize);
		}
		for (uint32_t i = 0; i < length; i++) {
			buffer[i] = data[i];
		}
	}

public:
	void getData(std::vector<u8> & buffer) {
		size_t length = data.size();
		buffer.resize(length);
		getData(&(buffer[0]), length);
	}

public:
	void getData(std::vector<u8> *buffer) {
		size_t length = data.size();
		buffer->resize(length);
		getData(&(buffer->at(0)), length);
	}

public:
	u8* getDataBufferAsArrayPointer() {
		return (data.size()!=0)? (u8*) (&data[0]):NULL;
	}

public:
	std::vector<u8>* getDataBuffer() {
		return &data;
	}

public:
	std::vector<u8>* getDataBufferAsVectorPointer() {
		return &data;
	}

public:
	u8 getDataCRC() const {
		return dataCRC;
	}

public:
	uint32_t getDataLength() const {
		return dataLength;
	}

public:
	uint32_t getLength() const {
		return dataLength;
	}

public:
	u8 getExtendedAddress() const {
		return extendedAddress;
	}

public:
	u8 getHeaderCRC() const {
		return headerCRC;
	}

public:
	u8 getInitiatorLogicalAddress() const {
		return initiatorLogicalAddress;
	}

public:
	u8 getInstruction() const {
		return instruction;
	}

public:
	u8 getKey() const {
		return key;
	}

public:
	std::vector<u8> getReplyAddress() const {
		return replyAddress;
	}

public:
	uint16_t getTransactionID() const {
		return transactionID;
	}

public:
	void setAddress(uint32_t address) {
		this->address = address;
	}

public:
	void setData(std::vector<u8> & data) {
		this->data = data;
		this->dataLength = data.size();
	}

public:
	void setData(u8 *data, size_t length) {
		this->data.clear();
		for (size_t i = 0; i < length; i++) {
			this->data.push_back(data[i]);
		}
		this->dataLength = length;
	}

public:
	void setDataCRC(u8 dataCRC) {
		this->dataCRC = dataCRC;
	}

public:
	void setDataLength(uint32_t dataLength) {
		this->dataLength = dataLength;
	}

public:
	void setLength(uint32_t dataLength) {
		this->dataLength = dataLength;
	}

public:
	void setExtendedAddress(u8 extendedAddress) {
		this->extendedAddress = extendedAddress;
	}

public:
	void setHeaderCRC(u8 headerCRC) {
		this->headerCRC = headerCRC;
	}

public:
	void setInitiatorLogicalAddress(u8 initiatorLogicalAddress) {
		this->initiatorLogicalAddress = initiatorLogicalAddress;
	}

public:
	void setInstruction(u8 instruction) {
		this->instruction = instruction;
	}

public:
	void setKey(u8 key) {
		this->key = key;
	}

public:
	void setReplyAddress(std::vector<u8> replyAddress, //
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

public:
	void setTransactionID(uint16_t transactionID) {
		this->transactionID = transactionID;
	}

public:
	u8 getStatus() const {
		return status;
	}

public:
	void setStatus(u8 status) {
		this->status = status;
	}

public:
	uint32_t getHeaderCRCMode() const {
		return headerCRCMode;
	}

public:
	void setHeaderCRCMode(uint32_t headerCRCMode) {
		this->headerCRCMode = headerCRCMode;
	}

public:
	uint32_t getDataCRCMode() const {
		return dataCRCMode;
	}

public:
	void setDataCRCMode(uint32_t dataCRCMode) {
		this->dataCRCMode = dataCRCMode;
	}

public:
	inline void addData(u8 oneByte) {
		this->data.push_back(oneByte);
	}

public:
	inline void clearData() {
		data.clear();
	}

public:
	inline void addData(std::vector<u8> array) {
		size_t size = array.size();
		for (size_t i = 0; i < size; i++) {
			data.push_back(array[i]);
		}
	}

public:
	std::string toString() {
		if (isCommand()) {
			return toStringCommandPacket();
		} else {
			return toStringReplyPacket();
		}
	}

public:
	std::string toXMLString() {
		if (isCommand()) {
			return toXMLStringCommandPacket();
		} else {
			return toXMLStringReplyPacket();
		}
	}

public:
	u8 getTargetLogicalAddress() const {
		return targetLogicalAddress;
	}

public:
	std::vector<u8> getTargetSpaceWireAddress() const {
		return targetSpaceWireAddress;
	}

public:
	void setTargetLogicalAddress(u8 targetLogicalAddress) {
		this->targetLogicalAddress = targetLogicalAddress;
	}

public:
	void setTargetSpaceWireAddress(std::vector<u8> targetSpaceWireAddress) {
		this->targetSpaceWireAddress = targetSpaceWireAddress;
	}

private:
	std::string toStringCommandPacket() {
		using namespace std;

		stringstream ss;
		///////////////////////////////
		//Command
		///////////////////////////////
		//Target SpaceWire Address
		if (targetSpaceWireAddress.size() != 0) {
			ss << "--------- Target SpaceWire Address ---------" << endl;
			SpaceWireUtilities::dumpPacket(&ss, &targetSpaceWireAddress, 1, 128);
		}
		//Header
		ss << "--------- RMAP Header Part ---------" << endl;
		//Initiator Logical Address
		ss << "Initiator Logical Address : 0x" << right << setw(2) << setfill('0') << hex
				<< (uint32_t) (initiatorLogicalAddress) << endl;
		//Target Logical Address
		ss << "Target Logic. Address     : 0x" << right << setw(2) << setfill('0') << hex
				<< (unsigned int) (targetLogicalAddress) << endl;
		//Protocol Identifier
		ss << "Protocol ID               : 0x" << right << setw(2) << setfill('0') << hex << (unsigned int) (1) << endl;
		//Instruction
		ss << "Instruction               : 0x" << right << setw(2) << setfill('0') << hex << (unsigned int) (instruction)
				<< endl;
		toStringInstructionField(ss);
		//Key
		ss << "Key                       : 0x" << setw(2) << setfill('0') << hex << (unsigned int) (key) << endl;
		//Reply Address
		if (replyAddress.size() != 0) {
			ss << "Reply Address             : ";
			SpaceWireUtilities::dumpPacket(&ss, &replyAddress, 1, 128);
		} else {
			ss << "Reply Address         : --none--" << endl;
		}
		ss << "Transaction Identifier    : 0x" << right << setw(4) << setfill('0') << hex << (unsigned int) (transactionID)
				<< endl;
		ss << "Extended Address          : 0x" << right << setw(2) << setfill('0') << hex
				<< (unsigned int) (extendedAddress) << endl;
		ss << "Address                   : 0x" << right << setw(8) << setfill('0') << hex << (unsigned int) (address)
				<< endl;
		ss << "Data Length (bytes)       : 0x" << right << setw(6) << setfill('0') << hex << (unsigned int) (dataLength)
				<< " (" << dec << dataLength << "dec)" << endl;
		ss << "Header CRC                : 0x" << right << setw(2) << setfill('0') << hex << (unsigned int) (headerCRC)
				<< endl;
		//Data Part
		ss << "---------  RMAP Data Part  ---------" << endl;
		if (isWrite()) {
			ss << "[data size = " << dec << dataLength << "bytes]" << endl;
			SpaceWireUtilities::dumpPacket(&ss, &data, 1, 16);
			ss << "Data CRC                  : " << right << setw(2) << setfill('0') << hex << (unsigned int) (dataCRC)
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

public:
	std::string toStringReplyPacket() {
		using namespace std;

		stringstream ss;
		this->constructPacket();
		///////////////////////////////
		//Reply
		///////////////////////////////
		ss << "RMAP Packet Dump" << endl;
		//Reply Address
		if (replyAddress.size() != 0) {
			ss << "--------- Reply Address ---------" << endl;
			ss << "Reply Address       : ";
			SpaceWireUtilities::dumpPacket(&ss, &replyAddress, 1, 128);
		}
		//Header
		ss << "--------- RMAP Header Part ---------" << endl;
		//Initiator Logical Address
		ss << "Initiator Logical Address : 0x" << right << setw(2) << setfill('0') << hex
				<< (uint32_t) (initiatorLogicalAddress) << endl;
		//Target Logical Address
		ss << "Target Logical Address    : 0x" << right << setw(2) << setfill('0') << hex
				<< (unsigned int) (targetLogicalAddress) << endl;
		//Protocol Identifier
		ss << "Protocol ID               : 0x" << right << setw(2) << setfill('0') << hex << (unsigned int) (1) << endl;
		//Instruction
		ss << "Instruction               : 0x" << right << setw(2) << setfill('0') << hex << (unsigned int) (instruction)
				<< endl;
		toStringInstructionField(ss);
		//Status
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
		ss << "Status                    : 0x" << right << setw(2) << setfill('0') << hex << (unsigned int) (status) << " ("
				<< statusstring << ")" << endl;
		ss << "Transaction Identifier    : 0x" << right << setw(4) << setfill('0') << hex << (unsigned int) (transactionID)
				<< endl;
		if (isRead()) {
			ss << "Data Length (bytes)       : 0x" << right << setw(6) << setfill('0') << hex << (unsigned int) (dataLength)
					<< " (" << dec << dataLength << "dec)" << endl;
		}
		ss << "Header CRC                : 0x" << right << setw(2) << setfill('0') << hex << (unsigned int) (headerCRC)
				<< endl;
		//Data Part
		ss << "---------  RMAP Data Part  ---------" << endl;
		if (isRead()) {
			ss << "[data size = " << dec << dataLength << "bytes]" << endl;
			SpaceWireUtilities::dumpPacket(&ss, &data, 1, 128);
			ss << "Data CRC    : 0x" << right << setw(2) << setfill('0') << hex << (unsigned int) (dataCRC) << endl;
		} else {
			ss << "--- none ---" << endl;
		}
		ss << endl;
		this->constructPacket();
		ss << "Total data (bytes)        : " << dec << this->getPacketBufferPointer()->size() << endl;
		ss << dec << endl;
		return ss.str();
	}

public:
	void toStringInstructionField(std::stringstream & ss) {
		using namespace std;

		//packet type (Command or Reply)
		ss << " ------------------------------" << endl;
		ss << " |Reserved    : 0" << endl;
		ss << " |Packet Type : " << (isCommand() ? 1 : 0);
		ss << " " << (isCommand() ? "(Command)" : "(Reply)") << endl;
		//Write or Read
		ss << " |Write/Read  : " << (isWrite() ? 1 : 0);
		ss << " " << (isWrite() ? "(Write)" : "(Read)") << endl;
		//Verify mode
		ss << " |Verify Mode : " << (isVerifyFlagSet() ? 1 : 0);
		ss << " " << (isVerifyFlagSet() ? "(Verify)" : "(No Verify)") << endl;
		//Ack mode
		ss << " |Reply Mode  : " << (isReplyFlagSet() ? 1 : 0);
		ss << " " << (isReplyFlagSet() ? "(Reply)" : "(No Reply)") << endl;
		//Increment
		ss << " |Increment   : " << (isIncrementFlagSet() ? 1 : 0);
		ss << " " << (isIncrementFlagSet() ? "(Increment)" : "(No Increment)") << endl;
		//SPAL
		ss << " |R.A.L.      : " << setw(1) << setfill('0') << hex
				<< (uint32_t) ((instruction & BitMaskForReplyPathAddressLength)) << endl;
		ss << " |(R.A.L. = Reply Address Length)" << endl;
		ss << " ------------------------------" << endl;
	}

public:
	std::string toXMLStringCommandPacket(int nTabs = 0) {
		this->constructPacket();
		using namespace std;

		stringstream ss;
		ss << "<RMAPPacket>" << endl;
		///////////////////////////////
		//Command
		///////////////////////////////
		//Protocol Identifier
		ss << "	<ProtocolID>" << "0x" << hex << right << setw(2) << setfill('0') << (uint32_t) (1) << "</ProtocolID>"
				<< endl;
		//SpaceWire Addresses
		ss << "	<InitiatorLogicalAddress>" << "0x" << hex << right << setw(2) << setfill('0')
				<< (uint32_t) (initiatorLogicalAddress) << "</InitiatorLogicalAddress>" << endl;
		ss << "	<TargetLogicalAddress>" << "0x" << hex << right << setw(2) << setfill('0')
				<< (uint32_t) (targetLogicalAddress) << "</TargetLogicalAddress>" << endl;
		ss << "	<TargetSpaceWireAddress>" << SpaceWireUtilities::packetToString(&targetSpaceWireAddress)
				<< "</TargetSpaceWireAddress>" << endl;
		ss << "	<ReplyAddress>" << SpaceWireUtilities::packetToString(&replyAddress) << "</ReplyAddress>" << endl;
		//Instruction
		ss << "	<Instruction>" << "0x" << hex << right << setw(2) << setfill('0') << (uint32_t) (instruction)
				<< "</Instruction>" << endl;
		//key
		ss << "	<Key>" << "0x" << hex << right << setw(2) << setfill('0') << (uint32_t) (key) << "</Key>" << endl;
		ss << "	<TransactionIdentifier>" << "0x" << hex << right << setw(2) << setfill('0')
				<< (unsigned int) (transactionID) << "</TransactionIdentifier>" << endl;
		ss << "	<ExtendedAddress>" << "0x" << hex << right << setw(2) << setfill('0') << (uint32_t) (extendedAddress)
				<< "</ExtendedAddress>" << endl;
		ss << "	<Address>" << "0x" << hex << right << setw(8) << setfill('0') << (uint32_t) (address) << "</Address>"
				<< endl;
		ss << "	<Length>" << "0x" << hex << right << setw(2) << setfill('0') << (uint32_t) (dataLength) << "</Length>"
				<< endl;
		if (headerCRCMode == AutoCRC) {
			ss << "	<HeaderCRC>Auto</HeaderCRC>" << endl;
			ss << "	<!-- HeaderCRC = " << "0x" << hex << right << setw(2) << setfill('0') << (uint32_t) (headerCRC)
					<< " (as long as the header is intact) -->" << endl;
		} else {
			ss << "	<HeaderCRC>" << "0x" << hex << right << setw(2) << setfill('0') << (uint32_t) (headerCRC)
					<< "</HeaderCRC>" << endl;
		}
		//Data Part
		if (isWrite()) {
			ss << "	<Data>" << endl;
			stringstream ssData;
			SpaceWireUtilities::dumpPacket(&ssData, &data, 1, 16);
			while (!ssData.eof()) {
				string line;
				getline(ssData, line);
				ss << "		" << line << endl;
			}
			ss << "	</Data>" << endl;
			if (dataCRCMode == AutoCRC) {
				ss << "	<DataCRC>Auto</DataCRC>" << endl;
				ss << "	<!-- DataCRC = " << "0x" << hex << right << setw(2) << setfill('0') << (uint32_t) (dataCRC)
						<< " (as long as the data part is intact) -->" << endl;
			} else {
				ss << "	<DataCRC>" << "0x" << hex << right << setw(2) << setfill('0') << (uint32_t) (dataCRC) << "</DataCRC>"
						<< endl;
			}
		}

		ss << "</RMAPPacket>" << endl;
		//indent
		stringstream ss2;
		while (!ss.eof()) {
			string line;
			getline(ss, line);
			for (int i = 0; i < nTabs; i++) {
				ss2 << "	";
			}
			ss2 << line << endl;
		}

		return ss2.str();
	}

public:
	std::string toXMLStringReplyPacket(int nTabs = 0) {
		this->constructPacket();
		using namespace std;

		this->constructPacket();
		stringstream ss;
		ss << "<RMAPPacket>" << endl;
		///////////////////////////////
		//Reply
		///////////////////////////////
		//Protocol Identifier
		ss << "	<ProtocolID>" << "0x" << hex << right << setw(2) << setfill('0') << (uint32_t) (1) << "</ProtocolID>"
				<< endl;
		//SpaceWire Addresses
		ss << "	<InitiatorLogicalAddress>" << "0x" << hex << right << setw(2) << setfill('0')
				<< (uint32_t) (initiatorLogicalAddress) << "</InitiatorLogicalAddress>" << endl;
		ss << "	<TargetLogicalAddress>" << "0x" << hex << right << setw(2) << setfill('0')
				<< (uint32_t) (targetLogicalAddress) << "</TargetLogicalAddress>" << endl;
		ss << "	<TargetSpaceWireAddress>" << SpaceWireUtilities::packetToString(&targetSpaceWireAddress)
				<< "</TargetSpaceWireAddress>" << endl;
		ss << "	<ReplyAddress>" << SpaceWireUtilities::packetToString(&replyAddress) << "</ReplyAddress>" << endl;
		//Instruction
		ss << "	<Instruction>" << "0x" << hex << right << setw(2) << setfill('0') << (uint32_t) (instruction)
				<< "</Instruction>" << endl;
		//Status
		ss << "	<Status>" << "0x" << hex << right << setw(2) << setfill('0') << (uint32_t) (status) << "</Status>" << endl;
		string statusstring;
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
		ss << "	<!--" << "Status 0x" << hex << right << setw(2) << setfill('0') << (uint32_t) (status) << "="
				<< statusstring << "-->" << endl;
		ss << "	<TransactionIdentifier>" << "0x" << hex << right << setw(2) << setfill('0')
				<< (unsigned int) (transactionID) << "</TransactionIdentifier>" << endl;
		ss << "	<ExtendedAddress>" << "0x" << hex << right << setw(2) << setfill('0') << (uint32_t) (extendedAddress)
				<< "</ExtendedAddress>" << endl;
		ss << "	<Address>" << "0x" << hex << right << setw(8) << setfill('0') << (uint32_t) (address) << "</Address>"
				<< endl;
		ss << "	<Length>" << "0x" << hex << right << setw(2) << setfill('0') << (uint32_t) (dataLength) << "</Length>"
				<< endl;
		if (headerCRCMode == AutoCRC) {
			ss << "	<HeaderCRC>Auto</HeaderCRC>" << endl;
			ss << "	<!-- HeaderCRC = " << "0x" << hex << right << setw(2) << setfill('0') << (uint32_t) (headerCRC)
					<< " (as long as the header is intact) -->" << endl;
		} else {
			ss << "	<HeaderCRC>" << "0x" << hex << right << setw(2) << setfill('0') << (uint32_t) (headerCRC)
					<< "</HeaderCRC>" << endl;
		}
		//Data Part
		if (isRead()) {
			ss << "	<Data>" << endl;
			stringstream ssData;
			SpaceWireUtilities::dumpPacket(&ssData, &data, 1, 16);
			while (!ssData.eof()) {
				string line;
				getline(ssData, line);
				ss << "		" << line << endl;
			}
			ss << "	</Data>" << endl;
			if (dataCRCMode == AutoCRC) {
				ss << "	<DataCRC>Auto</DataCRC>" << endl;
				ss << "	<!-- DataCRC = " << "0x" << hex << right << setw(2) << setfill('0') << (uint32_t) (dataCRC)
						<< " (as long as the data part is intact) -->" << endl;
			} else {
				ss << "	<DataCRC>" << "0x" << hex << right << setw(2) << setfill('0') << (uint32_t) (dataCRC) << "</DataCRC>"
						<< endl;
			}
		}

		ss << "</RMAPPacket>" << endl;

		//indent
		stringstream ss2;
		while (!ss.eof()) {
			string line;
			getline(ss, line);
			for (int i = 0; i < nTabs; i++) {
				ss2 << "	";
			}
			ss2 << line << endl;
		}
		return ss2.str();
	}

public:
	std::vector<u8>* getPacketBufferPointer() {
		constructPacket();
		return &wholePacket;
	}

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

public:
	/** Constructs a reply packet for a corresponding command packet.
	 * @param[in] commandPacket a command packet of which reply packet will be constructed
	 */
	static RMAPPacket* constructReplyForCommand(RMAPPacket* commandPacket, u8 status =
			RMAPReplyStatus::CommandExcecutedSuccessfully) {
		RMAPPacket* replyPacket = new RMAPPacket();
		*replyPacket = *commandPacket;

		//remove leading zeros in the Reply Address (Reply SpaceWire Address)
		replyPacket->setReplyAddress(removeLeadingZerosInReplyAddress(replyPacket->getReplyAddress()));

		replyPacket->clearData();
		replyPacket->setReply();
		replyPacket->setStatus(status);
		return replyPacket;
	}
};
#endif
