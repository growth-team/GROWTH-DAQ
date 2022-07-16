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
  write(ADDRESS_EVENT_OUTPUT_DISABLE_REGISTER, disableFlag);
  spdlog::debug("ConsumerManager::enableEventOutput() disable register readback = {}",
                read16(ADDRESS_EVENT_OUTPUT_DISABLE_REGISTER));
}

void ConsumerManagerEventFIFO::disableEventOutput() {
  constexpr u16 disableFlag = 0xFFFF;
  write(ADDRESS_EVENT_OUTPUT_DISABLE_REGISTER, disableFlag);
  spdlog::debug("ConsumerManager::enableEventOutput() disable register readback = {}",
                read16(ADDRESS_EVENT_OUTPUT_DISABLE_REGISTER));
}

void ConsumerManagerEventFIFO::getEventData(std::vector<u8>& receiveBuffer) {
  receiveBuffer.resize(RECEIVE_BUFFER_SIZE_BYTES);
  const auto receivedSize = readEventFIFO(receiveBuffer.data(), RECEIVE_BUFFER_SIZE_BYTES);
  receiveBuffer.resize(receivedSize);
  receivedBytes_ += receivedSize;
}

void ConsumerManagerEventFIFO::setEventPacket_NumberOfWaveform(u16 nSamples) {
  write(ADDRESS_EVENT_PACKET_NUMBER_OF_WAVEFORM_REGISTER, nSamples);
  spdlog::debug("setEventPacket_NumberOfWaveform write value = {:d}, readback = {:d}", nSamples,
                read16(ADDRESS_EVENT_PACKET_NUMBER_OF_WAVEFORM_REGISTER));
}

size_t ConsumerManagerEventFIFO::readEventFIFO(u8* buffer, size_t length) {
  const size_t totalReadSizeInBytes = [&]() {
    const size_t dataCountsInWords = read16(ADDRESS_EVENT_FIFO_DATA_COUNT_REGISTER);
    const size_t dataCountInBytes = dataCountsInWords * sizeof(u16);
    return std::min(length, dataCountInBytes);
  }();

  size_t receivedSizeInBytes = 0;
  while (receivedSizeInBytes < totalReadSizeInBytes) {
    const size_t remainingReadSizeInBytes = totalReadSizeInBytes - receivedSizeInBytes;
    const size_t singleReadSize = std::min(remainingReadSizeInBytes, SINGLE_READ_MAX_SIZE_BYTES);
    assert(singleReadSize != 0);
    read(INITIAL_ADDRESS_EVENT_FIFO, singleReadSize, buffer + receivedSizeInBytes);
    receivedSizeInBytes += singleReadSize;
  }
  return totalReadSizeInBytes;
}
