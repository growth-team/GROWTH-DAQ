#ifndef GROWTH_FPGA_EVENTDECODER_HH_
#define GROWTH_FPGA_EVENTDECODER_HH_

#include "types.h"
#include "growth-fpga/types.hh"

#include <array>
#include <deque>
#include <mutex>
#include <thread>
#include <optional>
#include <vector>

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
  EventDecoder();
  virtual ~EventDecoder();
  void reset();
  U8BufferPtr borrowU8Buffer();
  void pushEventPacketData(U8BufferPtr u8Buffer);
  std::optional<EventListPtr> getEventList();
  void returnEventList(EventListPtr&& eventList);

  /** Frees event instance so that buffer area can be reused in the following commands.
   * @param event event instance to be freed
   */
  void freeEvent(EventPtr event);

  /** Returns the number of available (allocated) Event instances.
   * @return the number of Event instances
   */
  size_t getNAllocatedEventInstances() const;

  void pauseEventDecoding() { pauseEventDecoding_ = true; }
  void resumeEventDecoding() { pauseEventDecoding_ = false; }

  static constexpr size_t INITIAL_NUM_EVENT_INSTANCES = 10000;

 private:
  void decodeThread();
  void decodeEvent(const std::vector<u8>* readDataUint8Array, EventList* currentEventList);
  void pushCurrentEventToEventList(EventList* eventList);

  std::deque<U8BufferPtr> u8BufferReservoir_;
  std::mutex u8BufferReservoirMutex_;

  std::deque<U8BufferPtr> u8BufferDecodeQueue_;
  std::mutex u8BufferDecodeQueueMutex_;

  std::condition_variable decodeQueueCondition_;
  std::mutex decodeQueueCVMutex_;

  std::deque<EventListPtr> eventListQueue_;
  std::mutex eventListQueueMutex_;

  std::deque<EventListPtr> eventListReservoir_;
  std::mutex eventListReservoirMutex_;

  EventDecoderState state_{};

  std::vector<u16> readDataUint16Array_{};
  std::deque<EventPtr> eventInstanceResavoir_{};
  std::mutex eventInstanceResavoirMutex_;
  size_t waveformLength_{};

  std::atomic<bool> pauseEventDecoding_{false};

  std::atomic<bool> stopDecodeThread_{false};
  std::thread decodeThread_;
};

#endif /* EVENTDECODER_HH_ */
