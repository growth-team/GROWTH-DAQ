#ifndef EVENTDECODER_HH_
#define EVENTDECODER_HH_

#include <deque>

#include "GROWTH_FY2015_ADCModules/Types.hh"

/** Decodes event data received from the GROWTH ADC Board.
 * Decoded event instances will be stored in a queue.
 */
class EventDecoder {
 private:
  struct RawEvent {
    uint8_t ch;
    uint8_t timeH;
    uint16_t timeM;
    uint16_t timeL;
    uint16_t triggerCount;
    uint16_t phaMax;
    uint16_t phaMaxTime;
    uint16_t phaMin;
    uint16_t phaFirst;
    uint16_t phaLast;
    uint16_t maxDerivative;
    uint16_t baseline;
    uint16_t* waveform;
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
  EventDecoder()
      : state(EventDecoderState::state_flag_FFF0),
        rawEvent(GROWTH_FY2015_ADC_Type::Event{GROWTH_FY2015_ADC_Type::MaxWaveformLength}) {
    for (size_t i = 0; i < InitialEventInstanceNumber; i++) {
      auto event = new GROWTH_FY2015_ADC_Type::Event{GROWTH_FY2015_ADC_Type::MaxWaveformLength};
      eventInstanceResavoir.push_back(event);
    }
  }

 public:
  virtual ~EventDecoder() {
    for (auto event : eventInstanceResavoir) {
      delete event;
    }
  }

 public:
  void decodeEvent(std::vector<uint8_t>* readDataUint8Array) {
    using namespace std;

    const size_t numBytes = readDataUint8Array->size();
    assert(numBytes % 2 == 0);
    const size_t numWords = numBytes / 2;

    // resize if necessary
    if (numWords > readDataUint16Array.size()) {
      readDataUint16Array.resize(numWords);
    }

    // fill data
    for (size_t i = 0; i < numWords; i++) {
      readDataUint16Array[i] = (readDataUint8Array->at((i << 1)) << 8) + readDataUint8Array->at((i << 1) + 1);
    }

    // decode the data
    // event packet format version 20151016
    for (size_t i = 0; i < numWords; i++) {
      switch (state) {
        case EventDecoderState::state_flag_FFF0:
          waveformLength = 0;
          if (readDataUint16Array[i] == 0xfff0) {
            state = EventDecoderState::state_ch_realtimeH;
          } else {
            cerr << "EventDecoder::decodeEvent(): invalid start flag ("
                 << "0x" << hex << right << setw(4) << setfill('0') << static_cast<uint32_t>(readDataUint16Array[i])
                 << ")" << endl;
          }
          break;
        case EventDecoderState::state_ch_realtimeH:
          rawEvent.ch = (readDataUint16Array[i] & 0xFF00) >> 8;
          rawEvent.timeH = readDataUint16Array[i] & 0xFF;
          state = EventDecoderState::state_realtimeM;
          break;
        case EventDecoderState::state_realtimeM:
          rawEvent.timeM = readDataUint16Array[i];
          state = EventDecoderState::state_realtimeL;
          break;
        case EventDecoderState::state_realtimeL:
          rawEvent.timeL = readDataUint16Array[i];
          state = EventDecoderState::state_reserved;
          break;
        case EventDecoderState::state_reserved:
          state = EventDecoderState::state_triggerCount;
          break;
        case EventDecoderState::state_triggerCount:
          rawEvent.triggerCount = readDataUint16Array[i];
          state = EventDecoderState::state_phaMax;
          break;
        case EventDecoderState::state_phaMax:
          rawEvent.phaMax = readDataUint16Array[i];
          state = EventDecoderState::state_phaMaxTime;
          break;
        case EventDecoderState::state_phaMaxTime:
          rawEvent.phaMaxTime = readDataUint16Array[i];
          state = EventDecoderState::state_phaMin;
          break;
        case EventDecoderState::state_phaMin:
          rawEvent.phaMin = readDataUint16Array[i];
          state = EventDecoderState::state_phaFirst;
          break;
        case EventDecoderState::state_phaFirst:
          rawEvent.phaFirst = readDataUint16Array[i];
          state = EventDecoderState::state_phaLast;
          break;
        case EventDecoderState::state_phaLast:
          rawEvent.phaLast = readDataUint16Array[i];
          state = EventDecoderState::state_maxDerivative;
          break;
        case EventDecoderState::state_maxDerivative:
          rawEvent.maxDerivative = readDataUint16Array[i];
          state = EventDecoderState::state_baseline;
          break;
        case EventDecoderState::state_baseline:
          rawEvent.baseline = readDataUint16Array[i];
          state = EventDecoderState::state_pha_list;
          break;
        case EventDecoderState::state_pha_list:
          if (readDataUint16Array[i] == 0xFFFF) {
            // push GROWTH_FY2015_ADC_Type::Event to a queue
            pushEventToQueue();
            // move to the idle state
            state = EventDecoderState::state_flag_FFF0;
            break;
          } else {
            if (GROWTH_FY2015_ADC_Type::MaxWaveformLength <= waveformLength) {
              cerr << "EventDecoder::decodeEvent(): waveform too long. something is wrong with data transfer. Return "
                      "to the idle state."
                   << endl;
              state = EventDecoderState::state_flag_FFF0;
            } else {
              rawEvent.waveform[waveformLength] = readDataUint16Array[i];
              waveformLength++;
            }
          }
          break;
      }
    }
  }

 public:
  static const size_t InitialEventInstanceNumber = 10000;

 public:
  void pushEventToQueue() {
    GROWTH_FY2015_ADC_Type::Event* event;
    if (eventInstanceResavoir.size() == 0) {
      event = new GROWTH_FY2015_ADC_Type::Event{GROWTH_FY2015_ADC_Type::MaxWaveformLength};
    } else {
      // reuse already created instance
      event = eventInstanceResavoir.back();
      eventInstanceResavoir.pop_back();
    }
    event->ch = rawEvent.ch;
    event->timeTag = (static_cast<uint64_t>(rawEvent.timeH) << 32) + (static_cast<uint64_t>(rawEvent.timeM) << 16) +
                     (rawEvent.timeL);
    event->phaMax = rawEvent.phaMax;
    event->phaMaxTime = rawEvent.phaMaxTime;
    event->phaMin = rawEvent.phaMin;
    event->phaFirst = rawEvent.phaFirst;
    event->phaLast = rawEvent.phaLast;
    event->maxDerivative = rawEvent.maxDerivative;
    event->baseline = rawEvent.baseline;
    event->nSamples = waveformLength;
    event->triggerCount = rawEvent.triggerCount;

    // copy waveform
    const size_t nBytes = waveformLength << 1;
    memcpy(event->waveform, rawEvent.waveform, nBytes);

    eventQueue.push_back(event);
  }

 public:
  /** Returns decoded event queue (as std::vecotr).
   * After used in user application, decoded events should be freed
   * via EventDecoder::freeEvent(GROWTH_FY2015_ADC_Type::Event* event).
   * @return std::queue containing pointers to decoded events
   */
  std::vector<GROWTH_FY2015_ADC_Type::Event*> getDecodedEvents() {
    std::vector<GROWTH_FY2015_ADC_Type::Event*> eventQueueCopied = eventQueue;
    eventQueue.clear();
    return eventQueueCopied;
  }

 public:
  /** Frees event instance so that buffer area can be reused in the following commands.
   * @param event event instance to be freed
   */
  void freeEvent(GROWTH_FY2015_ADC_Type::Event* event) { eventInstanceResavoir.push_back(event); }

 public:
  /** Returns the number of available (allocated) Event instances.
   * @return the number of Event instances
   */
  size_t getNAllocatedEventInstances() { return eventInstanceResavoir.size(); }

 public:
 public:
  std::string stateToString() {
    std::string result;
    switch (state) {
      case EventDecoderState::state_flag_FFF0:
        result = "state_flag_FFF0";
        break;
      case EventDecoderState::state_ch_realtimeH:
        result = "state_ch_realtimeH";
        break;
      case EventDecoderState::state_realtimeM:
        result = "state_realtimeM";
        break;
      case EventDecoderState::state_realtimeL:
        result = "state_realtimeL";
        break;
      case EventDecoderState::state_reserved:
        result = "state_reserved";
        break;
      case EventDecoderState::state_triggerCount:
        result = "state_triggerCount";
        break;
      case EventDecoderState::state_phaMax:
        result = "state_phaMax";
        break;
      case EventDecoderState::state_phaMaxTime:
        result = "state_phaMaxTime";
        break;
      case EventDecoderState::state_phaMin:
        result = "state_phaMin";
        break;
      case EventDecoderState::state_phaFirst:
        result = "state_phaFirst";
        break;
      case EventDecoderState::state_phaLast:
        result = "state_phaLast";
        break;
      case EventDecoderState::state_maxDerivative:
        result = "state_maxDerivative";
        break;
      case EventDecoderState::state_baseline:
        result = "state_baseline";
        break;
      case EventDecoderState::state_pha_list:
        result = "state_pha_list";
        break;

      default:
        result = "Undefined status";
        break;
    }
    return result;
  }

 private:
  EventDecoderState state{};
  std::vector<GROWTH_FY2015_ADC_Type::Event*> eventQueue;
  std::vector<uint16_t> readDataUint16Array;
  std::deque<GROWTH_FY2015_ADC_Type::Event*> eventInstanceResavoir;
  size_t waveformLength{};
};

#endif /* EVENTDECODER_HH_ */
