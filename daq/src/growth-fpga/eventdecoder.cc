#include "growth-fpga/eventdecoder.hh"
#include "spdlog/spdlog.h"

#include <cassert>
#include <deque>
#include <chrono>
#include <cstring>

EventDecoder::EventDecoder() : state_(EventDecoderState::state_flag_FFF0), rawEvent{} {
  for (size_t i = 0; i < INITIAL_NUM_EVENT_INSTANCES; i++) {
    EventPtr event = std::make_unique<growth_fpga::Event>(growth_fpga::MaxWaveformLength);
    eventInstanceResavoir_.push_back(std::move(event));
  }
  stopDecodeThread_ = false;
  decodeThread_ = std::thread(&EventDecoder::decodeThread, this);
}

EventDecoder::~EventDecoder() {
  spdlog::info("Waiting for EventDecoder thread to join");
  stopDecodeThread_ = true;
  decodeThread_.join();
  spdlog::info("EventDecoder thread joined");
}

void EventDecoder::reset() {
  {
    std::lock_guard<std::mutex> guard(u8BufferDecodeQueueMutex_);
    u8BufferDecodeQueue_.clear();
  }
  {
    std::lock_guard<std::mutex> guard(eventListQueueMutex_);
    eventListQueue_.clear();
  }
  state_ = EventDecoderState::state_flag_FFF0;
}

U8BufferPtr EventDecoder::borrowU8Buffer() {
  std::lock_guard<std::mutex> guard(u8BufferReservoirMutex_);
  if (!u8BufferReservoir_.empty()) {
    U8BufferPtr result = std::move(u8BufferReservoir_.front());
    u8BufferReservoir_.pop_front();
    return result;
  } else {
    return std::make_unique<U8Buffer>();
  }
}

void EventDecoder::pushEventPacketData(U8BufferPtr u8Buffer) {
  std::lock_guard<std::mutex> guard(u8BufferDecodeQueueMutex_);
  // Push the buffer to the decode queue
  u8BufferDecodeQueue_.emplace_back(std::move(u8Buffer));
  // Notify the decode thread
  decodeQueueCondition_.notify_one();
}

std::optional<EventListPtr> EventDecoder::getEventList() {
  if (eventListQueue_.empty()) {
    return {};
  }
  std::lock_guard<std::mutex> guard(eventListQueueMutex_);
  auto result = std::move(eventListQueue_.front());
  eventListQueue_.pop_front();
  return result;
}

void EventDecoder::returnEventList(EventListPtr&& eventList) {
  for (auto& event : *eventList) {
    freeEvent(std::move(event));
  }
  eventList->clear();
  eventListReservoir_.emplace_back(std::move(eventList));
}

void EventDecoder::freeEvent(EventPtr event) {
  std::lock_guard<std::mutex> guard(eventInstanceResavoirMutex_);
  eventInstanceResavoir_.emplace_back(std::move(event));
}

size_t EventDecoder::getNAllocatedEventInstances() const { return eventInstanceResavoir_.size(); }

void EventDecoder::decodeThread() {
  spdlog::info("EventPacketDecodeThread started...");
  std::unique_lock<std::mutex> lock(decodeQueueCVMutex_);
  std::vector<U8BufferPtr> localU8BufferList;
  std::vector<U8BufferPtr> localU8BufferListDecoded;
  EventListPtr currentEventList{};
  while (!stopDecodeThread_) {
    decodeQueueCondition_.wait_for(lock, std::chrono::milliseconds(500),
                                   [&]() { return !u8BufferDecodeQueue_.empty(); });
    // Pop u8Buffer
    {
      if (u8BufferDecodeQueue_.empty()) {
        spdlog::debug("EventDecoder::decodeThread() u8BufferDecodeQueue is empty()");
        continue;
      }
      std::lock_guard<std::mutex> guard(u8BufferDecodeQueueMutex_);
      while (!u8BufferDecodeQueue_.empty()) {
        localU8BufferList.emplace_back(std::move(u8BufferDecodeQueue_.front()));
        u8BufferDecodeQueue_.pop_front();
      }
    }
    // Decode
    if (!currentEventList) {
      currentEventList = [&]() {
        std::lock_guard<std::mutex> guard(eventListReservoirMutex_);
        if (!eventListReservoir_.empty()) {
          auto eventListPtr = std::move(eventListReservoir_.front());
          eventListReservoir_.pop_front();
          return eventListPtr;
        } else {
          return std::make_unique<EventList>();
        }
      }();
    }
    if (!pauseEventDecoding_) {
      for (auto& u8Buffer : localU8BufferList) {
        decodeEvent(u8Buffer.get(), currentEventList.get());
      }
    }
    if (!currentEventList->empty()) {
      std::lock_guard<std::mutex> guard(eventListQueueMutex_);
      eventListQueue_.emplace_back(std::move(currentEventList));
      currentEventList.reset();
    }
    // Push u8Buffer to reservoir
    {
      std::lock_guard<std::mutex> guard(u8BufferReservoirMutex_);
      for (auto& u8Buffer : localU8BufferList) {
        u8BufferReservoir_.emplace_back(std::move(u8Buffer));
      }
    }
    localU8BufferList.clear();
  }
  spdlog::info("EventPacketDecodeThread stopped.");
}

void EventDecoder::decodeEvent(const std::vector<u8>* readDataUint8Array, EventList* currentEventList) {
  bool dumpInvalidHeaderError = true;
  size_t invalidHeaderErrorCount = 0;
  constexpr size_t MAX_NUM_INVALID_HEADER_ERROR = 5;

  const size_t numBytes = readDataUint8Array->size();
  assert(numBytes % 2 == 0);
  const size_t numWords = numBytes / 2;
  readDataUint16Array_.resize(numWords);

  // fill data
  for (size_t i = 0; i < numWords; i++) {
    readDataUint16Array_[i] = (readDataUint8Array->at((i << 1)) << 8) + readDataUint8Array->at((i << 1) + 1);
  }

  // decode the data
  // event packet format version 20151016
  for (size_t i = 0; i < numWords; i++) {
    switch (state_) {
      case EventDecoderState::state_flag_FFF0:
        waveformLength_ = 0;
        if (readDataUint16Array_[i] == 0xfff0) {
          state_ = EventDecoderState::state_ch_realtimeH;
        } else {
          if (invalidHeaderErrorCount < MAX_NUM_INVALID_HEADER_ERROR) {
            spdlog::warn("EventDecoder::decodeEvent(): invalid start flag (0x{:04x})",
                         static_cast<u32>(readDataUint16Array_[i]));
          }
          invalidHeaderErrorCount++;
        }
        break;
      case EventDecoderState::state_ch_realtimeH:
        rawEvent.ch = (readDataUint16Array_[i] & 0xFF00) >> 8;
        rawEvent.timeH = readDataUint16Array_[i] & 0xFF;
        state_ = EventDecoderState::state_realtimeM;
        break;
      case EventDecoderState::state_realtimeM:
        rawEvent.timeM = readDataUint16Array_[i];
        state_ = EventDecoderState::state_realtimeL;
        break;
      case EventDecoderState::state_realtimeL:
        rawEvent.timeL = readDataUint16Array_[i];
        state_ = EventDecoderState::state_reserved;
        break;
      case EventDecoderState::state_reserved:
        state_ = EventDecoderState::state_triggerCount;
        break;
      case EventDecoderState::state_triggerCount:
        rawEvent.triggerCount = readDataUint16Array_[i];
        state_ = EventDecoderState::state_phaMax;
        break;
      case EventDecoderState::state_phaMax:
        rawEvent.phaMax = readDataUint16Array_[i];
        state_ = EventDecoderState::state_phaMaxTime;
        break;
      case EventDecoderState::state_phaMaxTime:
        rawEvent.phaMaxTime = readDataUint16Array_[i];
        state_ = EventDecoderState::state_phaMin;
        break;
      case EventDecoderState::state_phaMin:
        rawEvent.phaMin = readDataUint16Array_[i];
        state_ = EventDecoderState::state_phaFirst;
        break;
      case EventDecoderState::state_phaFirst:
        rawEvent.phaFirst = readDataUint16Array_[i];
        state_ = EventDecoderState::state_phaLast;
        break;
      case EventDecoderState::state_phaLast:
        rawEvent.phaLast = readDataUint16Array_[i];
        state_ = EventDecoderState::state_maxDerivative;
        break;
      case EventDecoderState::state_maxDerivative:
        rawEvent.maxDerivative = readDataUint16Array_[i];
        state_ = EventDecoderState::state_baseline;
        break;
      case EventDecoderState::state_baseline:
        rawEvent.baseline = readDataUint16Array_[i];
        state_ = EventDecoderState::state_pha_list;
        break;
      case EventDecoderState::state_pha_list:
        if (readDataUint16Array_[i] == 0xFFFF) {
          pushCurrentEventToEventList(currentEventList);
          // move to the idle state
          state_ = EventDecoderState::state_flag_FFF0;
          break;
        } else {
          if (growth_fpga::MaxWaveformLength <= waveformLength_) {
            spdlog::error(
                "EventDecoder::decodeEvent(): waveform too long. something is wrong with data transfer. Return to "
                "the idle state.");
            state_ = EventDecoderState::state_flag_FFF0;
          } else {
            rawEvent.waveform[waveformLength_] = readDataUint16Array_[i];
            waveformLength_++;
          }
        }
        break;
    }
  }
  if (invalidHeaderErrorCount > MAX_NUM_INVALID_HEADER_ERROR) {
    spdlog::warn("There were {} more invalid header values detected",
                 static_cast<size_t>(invalidHeaderErrorCount - MAX_NUM_INVALID_HEADER_ERROR));
  }
}
void EventDecoder::pushCurrentEventToEventList(EventList* eventList) {
  EventPtr event{};
  if (eventInstanceResavoir_.empty()) {
    event = std::make_unique<growth_fpga::Event>(growth_fpga::MaxWaveformLength);
  } else {
    // reuse already created instance
    std::lock_guard<std::mutex> guard(eventInstanceResavoirMutex_);
    event = std::move(eventInstanceResavoir_.back());
    eventInstanceResavoir_.pop_back();
  }
  event->ch = rawEvent.ch;
  event->timeTag =
      (static_cast<u64>(rawEvent.timeH) << 32) + (static_cast<u64>(rawEvent.timeM) << 16) + (rawEvent.timeL);
  event->phaMax = rawEvent.phaMax;
  event->phaMaxTime = rawEvent.phaMaxTime;
  event->phaMin = rawEvent.phaMin;
  event->phaFirst = rawEvent.phaFirst;
  event->phaLast = rawEvent.phaLast;
  event->maxDerivative = rawEvent.maxDerivative;
  event->baseline = rawEvent.baseline;
  event->nSamples = waveformLength_;
  event->triggerCount = rawEvent.triggerCount;

  // copy waveform
  const size_t nBytes = waveformLength_ << 1;
  memcpy(event->waveform, rawEvent.waveform.data(), nBytes);

  eventList->push_back(std::move(event));
}
