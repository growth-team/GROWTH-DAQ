#ifndef GROWTH_FPGA_EVENTDECODER_HH_
#define GROWTH_FPGA_EVENTDECODER_HH_

#include "types.h"
#include "growth-fpga/types.hh"
#include "spdlog/spdlog.h"

#include <cassert>
#include <deque>
#include <chrono>

/** Decodes event data received from the GROWTH ADC Board.
 * Decoded event instances will be stored in a queue.
 */
class EventDecoder {
 private:
  struct RawEvent {
    u8 ch{};
    u8 timeH{};
    u16 timeM{};
    u16 timeL{};
    u16 triggerCount{};
    u16 phaMax{};
    u16 phaMaxTime{};
    u16 phaMin{};
    u16 phaFirst{};
    u16 phaLast{};
    u16 maxDerivative{};
    u16 baseline{};
    std::array<u16, growth_fpga::MaxWaveformLength> waveform{};
  } rawEvent;

 private:
  enum class EventDecoderState {
    state_flag_FFF0,
    state_ch_realtimeH,
    state_realtimeM,
    state_realtimeL,
    state_reserved,
    state_triggerCount,
    state_phaMax,
    state_phaMaxTime,
    state_phaMin,
    state_phaFirst,
    state_phaLast,
    state_maxDerivative,
    state_baseline,
    state_pha_list
  };

 public:
  EventDecoder() : state_(EventDecoderState::state_flag_FFF0), rawEvent{} {
    for (size_t i = 0; i < INITIAL_NUM_EVENT_INSTANCES; i++) {
      EventPtr event = std::make_unique<growth_fpga::Event>(growth_fpga::MaxWaveformLength);
      eventInstanceResavoir_.push_back(std::move(event));
    }
    stopDecodeThread_ = false;
    decodeThread_ = std::thread(&EventDecoder::decodeThread, this);
  }

  virtual ~EventDecoder() {
    stopDecodeThread_ = true;
    decodeThread_.join();
  }

  void reset() {
    {
      std::lock_guard<std::mutex> guard(u8BufferDecodeQueueMutex_);
      u8BufferDecodeQueue_.clear();
    }
    {
      std::lock_guard<std::mutex> guard(eventListQueueMutex);
      eventListQueue_.clear();
    }
    state_ = EventDecoderState::state_flag_FFF0;
  }

  U8BufferPtr borrowU8Buffer() {
    std::lock_guard<std::mutex> guard(u8BufferReservoirMutex_);
    if (u8BufferReservoir_.empty()) {
      return std::make_unique<U8Buffer>();
    }
    auto result = std::move(u8BufferReservoir_.front());
    u8BufferReservoir_.pop_front();
    return result;
  }

  void pushEventPacketData(U8BufferPtr&& u8Buffer) {
    std::lock_guard<std::mutex> guard(u8BufferDecodeQueueMutex_);
    // Push the buffer to the decode queue
    u8BufferDecodeQueue_.emplace_back(std::move(u8Buffer));
    // Notify the decode thread
    decodeQueueCondition_.notify_one();
  }

  std::optional<EventListPtr> getEventList() {
    if (eventListQueue_.empty()) {
      return {};
    }
    std::lock_guard<std::mutex> guard(eventListQueueMutex);
    auto result = std::move(eventListQueue_.front());
    eventListQueue_.pop_front();
    return result;
  }

  void returnEventList(EventListPtr&& eventList) {
    for (auto& event : *eventList) {
      freeEvent(std::move(event));
    }
    eventList->clear();
    eventListReservoir_.push_back(std::move(eventList));
  }

  /** Frees event instance so that buffer area can be reused in the following commands.
   * @param event event instance to be freed
   */
  void freeEvent(EventPtr&& event) {
    std::lock_guard<std::mutex> guard(eventInstanceResavoirMutex_);
    eventInstanceResavoir_.push_back(std::move(event));
  }

  /** Returns the number of available (allocated) Event instances.
   * @return the number of Event instances
   */
  size_t getNAllocatedEventInstances() const { return eventInstanceResavoir_.size(); }

  static constexpr size_t INITIAL_NUM_EVENT_INSTANCES = 10000;

 private:
  void decodeThread() {
    spdlog::info("EventPacketDecodeThread started...");
    std::unique_lock<std::mutex> lock(decodeQueueCVMutex_);
    std::vector<U8BufferPtr> localU8BufferList;
    EventListPtr currentEventList{};
    while (!stopDecodeThread_) {
      decodeQueueCondition_.wait_for(lock, std::chrono::milliseconds(100),
                                     [&]() { return !u8BufferDecodeQueue_.empty(); });
      // Pop u8Buffer
      {
        std::lock_guard<std::mutex> guard(u8BufferDecodeQueueMutex_);
        if(u8BufferDecodeQueue_.empty()){
          spdlog::info("u8BufferDecodeQueue is empty()");
          continue;
        }
        localU8BufferList.push_back(std::move(u8BufferDecodeQueue_.front()));
        u8BufferDecodeQueue_.pop_front();
      }
      // Decode
      if (!currentEventList) {
        currentEventList = [&]() {
          std::lock_guard<std::mutex> guard(eventListReservoirMutex);
          if (eventListReservoir_.empty()) {
            auto eventListPtr = std::move(eventListReservoir_.front());
            eventListReservoir_.pop_front();
            return eventListPtr;
          } else {
            return std::make_unique<EventList>();
          }
        }();
      }
      for (auto& u8Buffer : localU8BufferList) {
        decodeEvent(u8Buffer.get(), currentEventList.get());
        u8BufferReservoir_.push_back(std::move(u8Buffer));
      }
      if (!currentEventList->empty()) {
        std::lock_guard<std::mutex> guard(eventListQueueMutex);
        eventListQueue_.push_back(std::move(currentEventList));
        currentEventList.reset();
      }
      // Push u8Buffer to reservoir
      {
        std::lock_guard<std::mutex> guard(u8BufferReservoirMutex_);
        for (auto& u8Buffer : localU8BufferList) {
          u8BufferReservoir_.push_back(std::move(u8Buffer));
        }
      }
    }
    spdlog::info("EventPacketDecodeThread stopped.");
  }

  void decodeEvent(const std::vector<u8>* readDataUint8Array, EventList* currentEventList) {
    bool dumpInvalidHeaderError = true;
    size_t invalidHeaderErrorCount = 0;
    constexpr size_t MAX_NUM_INVALID_HEADER_ERROR = 5;

    const size_t numBytes = readDataUint8Array->size();
    assert(numBytes % 2 == 0);
    const size_t numWords = numBytes / 2;
    readDataUint16Array_.reserve(numWords);

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
  void pushCurrentEventToEventList(EventList* eventList) {
    EventPtr event;
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

  std::deque<U8BufferPtr> u8BufferReservoir_;
  std::mutex u8BufferReservoirMutex_;

  std::deque<U8BufferPtr> u8BufferDecodeQueue_;
  std::mutex u8BufferDecodeQueueMutex_;

  std::condition_variable decodeQueueCondition_;
  std::mutex decodeQueueCVMutex_;

  std::deque<EventListPtr> eventListQueue_;
  std::mutex eventListQueueMutex;

  std::deque<EventListPtr> eventListReservoir_;
  std::mutex eventListReservoirMutex;

  EventDecoderState state_{};

  std::vector<u16> readDataUint16Array_{};
  std::deque<EventPtr> eventInstanceResavoir_{};
  std::mutex eventInstanceResavoirMutex_;
  size_t waveformLength_{};

  std::atomic<bool> stopDecodeThread_{false};
  std::thread decodeThread_;
};

#endif /* EVENTDECODER_HH_ */
