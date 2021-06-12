#include "growth-fpga/consumermanagereventfifo.hh"

#include <cassert>

ConsumerManagerEventFIFO::ConsumerManagerEventFIFO(RMAPInitiatorPtr rmapInitiator,
                                                   std::shared_ptr<RMAPTargetNode> rmapTargetNode)
    : RegisterAccessInterface(std::move(rmapInitiator), rmapTargetNode) {}

ConsumerManagerEventFIFO::~ConsumerManagerEventFIFO() {
  if (eventDataReadLoopThread_.joinable()) {
    eventDataReadLoopStop_ = true;
    eventDataReadLoopThread_.join();
  }
}

void ConsumerManagerEventFIFO::enableEventOutput() {
  constexpr u16 disableFlag = 0;
  write(AddressOf_EventOutputDisableRegister, disableFlag);
  spdlog::debug("ConsumerManager::enableEventOutput() disable register readback = {}",
                read16(AddressOf_EventOutputDisableRegister));
}

void ConsumerManagerEventFIFO::disableEventOutput() {
  constexpr u16 disableFlag = 0xFFFF;
  write(AddressOf_EventOutputDisableRegister, disableFlag);
  spdlog::debug("ConsumerManager::enableEventOutput() disable register readback = {}",
                read16(AddressOf_EventOutputDisableRegister));
}

void ConsumerManagerEventFIFO::getEventData(std::vector<u8>& receiveBuffer) {
  receiveBuffer.resize(RECEIVE_BUFFER_SIZE_BYTES);
  const auto receivedSize = readEventFIFO(receiveBuffer.data(), RECEIVE_BUFFER_SIZE_BYTES);
  receiveBuffer.resize(receivedSize);
  receivedBytes_ += receivedSize;
}

void ConsumerManagerEventFIFO::setEventPacket_NumberOfWaveform(u16 nSamples) {
  write(AddressOf_EventPacket_NumberOfWaveform_Register, nSamples);
  spdlog::debug("setEventPacket_NumberOfWaveform write value = {:d}, readback = {:d}", nSamples,
                read16(AddressOf_EventPacket_NumberOfWaveform_Register));
}

size_t ConsumerManagerEventFIFO::readEventFIFO(u8* buffer, size_t length) {
  const size_t totalReadSizeInBytes = [&]() {
    const size_t dataCountsInWords = read16(AddressOf_EventFIFO_DataCount_Register);
    const size_t dataCountInBytes = dataCountsInWords * sizeof(u16);
    return std::min(length, dataCountInBytes);
  }();

  size_t receivedSizeInBytes = 0;
  while (receivedSizeInBytes < totalReadSizeInBytes) {
    const size_t remainingReadSizeInBytes = totalReadSizeInBytes - receivedSizeInBytes;
    const size_t singleReadSize = std::min(remainingReadSizeInBytes, SINGLE_READ_MAX_SIZE_BYTES);
    assert(singleReadSize != 0);
    read(InitialAddressOf_EventFIFO, singleReadSize, buffer + receivedSizeInBytes);
    receivedSizeInBytes += singleReadSize;
  }
  return totalReadSizeInBytes;
}
