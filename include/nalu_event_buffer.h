#ifndef NALU_EVENT_BUFFER_H
#define NALU_EVENT_BUFFER_H

#include <vector>
#include <functional>
#include <memory>
#include <mutex>  // For thread safety
#include "nalu_event.h"
#include "nalu_time_difference_calculator.h"

class NaluEventBuffer {
public:
    NaluEventBuffer(size_t max_events, NaluTimeDifferenceCalculator& time_diff_calculator, size_t max_lookback, size_t max_event_size, uint16_t event_header, uint16_t event_trailer);
    ~NaluEventBuffer();

    void add_event(std::unique_ptr<NaluEvent> event);
    std::vector<std::unique_ptr<NaluEvent>>& get_events();
    NaluEvent& get_latest_event();
    NaluEvent& get_event_by_index(size_t index);

    void set_on_overflow_callback(std::function<void()> callback);
    void set_max_events(size_t max_events);

    std::vector<NaluEvent*> get_events_after_timestamp(const std::chrono::steady_clock::time_point& timestamp, ssize_t seed_index = -1) const;
    size_t remove_events_before_timestamp(const std::chrono::steady_clock::time_point& timestamp, ssize_t seed_index = -1);

    std::vector<NaluEvent*> get_events_after_index_inclusive(size_t index) const;
    size_t remove_events_before_index_exclusive(size_t index);

    void add_packet(const NaluPacket& packet, bool& in_safety_buffer_zone, uint32_t& event_index);

private:
    mutable std::mutex buffer_mutex;  // Mutex to protect event buffer
    std::vector<std::unique_ptr<NaluEvent>> events;
    size_t max_events;
    std::function<void()> overflow_callback;

    // Member variables for configuration
    NaluTimeDifferenceCalculator& time_diff_calculator;
    size_t max_lookback;
    size_t max_event_size;
    uint16_t event_header;
    uint16_t event_trailer;
};

#endif // NALU_EVENT_BUFFER_H
