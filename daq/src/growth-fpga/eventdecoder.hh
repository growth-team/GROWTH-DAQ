#ifndef GROWTH_FPGA_EVENTDECODER_HH_
#define GROWTH_FPGA_EVENTDECODER_HH_

#include "types.h"
#include "growth-fpga/types.hh"

#include <cassert>
#include <deque>


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
  /** Constructor.
   */
  EventDecoder() : state_(EventDecoderState::state_flag_FFF0), rawEvent{} {
    for (size_t i = 0; i < INITIAL_NUM_EVENT_INSTANCES; i++) {
      auto event = new growth_fpga::Event{growth_fpga::MaxWaveformLength};
      eventInstanceResavoir_.push_back(event);
    }
  }

  virtual ~EventDecoder() {
    for (auto event : eventInstanceResavoir_) {
      delete event;
    }
  }

  void decodeEvent(const std::vector<u8>* readDataUint8Array) {
    using namespace std;

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
            cerr << "EventDecoder::decodeEvent(): invalid start flag ("
                 << "0x" << hex << right << setw(4) << setfill('0') << static_cast<u32>(readDataUint16Array_[i]) << ")"
                 << endl;
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
            // push GROWTH_FY2015_ADC_Type::Event to a queue
            pushEventToQueue();
            // move to the idle state
            state_ = EventDecoderState::state_flag_FFF0;
            break;
          } else {
            if (growth_fpga::MaxWaveformLength <= waveformLength_) {
              cerr << "EventDecoder::decodeEvent(): waveform too long. something is wrong with data transfer. Return "
                      "to the idle state."
                   << endl;
              state_ = EventDecoderState::state_flag_FFF0;
            } else {
              rawEvent.waveform[waveformLength_] = readDataUint16Array_[i];
              waveformLength_++;
            }
          }
          break;
      }
    }
  }

  void pushEventToQueue() {
    growth_fpga::Event* event;
    if (eventInstanceResavoir_.empty()) {
      event = new growth_fpga::Event{growth_fpga::MaxWaveformLength};
    } else {
      // reuse already created instance
      event = eventInstanceResavoir_.back();
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

    eventQueue_.push_back(event);
  }

  /** Returns decoded event queue (as std::vector).
   * After used in user application, decoded events should be freed
   * via EventDecoder::freeEvent(GROWTH_FY2015_ADC_Type::Event* event).
   * @return std::queue containing pointers to decoded events
   */
  std::vector<growth_fpga::Event*> getDecodedEvents() {
    std::vector<growth_fpga::Event*> eventQueueCopied = eventQueue_;
    eventQueue_.clear();
    return eventQueueCopied;
  }

  /** Frees event instance so that buffer area can be reused in the following commands.
   * @param event event instance to be freed
   */
  void freeEvent(growth_fpga::Event* event) { eventInstanceResavoir_.push_back(event); }

  /** Returns the number of available (allocated) Event instances.
   * @return the number of Event instances
   */
  size_t getNAllocatedEventInstances() const { return eventInstanceResavoir_.size(); }

  static constexpr size_t INITIAL_NUM_EVENT_INSTANCES = 10000;

 private:
  EventDecoderState state_{};
  std::vector<growth_fpga::Event*> eventQueue_{};
  std::vector<u16> readDataUint16Array_{};
  std::deque<growth_fpga::Event*> eventInstanceResavoir_{};
  size_t waveformLength_{};
};

#endif /* EVENTDECODER_HH_ */
