#ifndef NALU_EVENT_BUFFER_H
#define NALU_EVENT_BUFFER_H

#include <vector>
#include <functional>  // For user-defined callback
#include "nalu_event.h"
#include <memory>  // For smart pointers

class NaluEventBuffer {
public:
    // Constructor with max_events directly
    NaluEventBuffer(size_t max_events = 100);

    // Destructor automatically handles cleanup
    ~NaluEventBuffer();

    // Adds a new event to the buffer (use unique_ptr now)
    void add_event(std::unique_ptr<NaluEvent> event);

    // Returns a reference to the stored events
    std::vector<std::unique_ptr<NaluEvent>>& get_events();

    // Returns a reference to the latest event
    NaluEvent& get_latest_event();

    // Returns a reference to the event by index
    NaluEvent& get_event_by_index(size_t index);

    // Set a callback for buffer overflow
    void set_on_overflow_callback(std::function<void()> callback);

    // Set a new max number of events in the buffer
    void set_max_events(size_t max_events);

    // Get all events after a certain timestamp
    std::vector<NaluEvent*> get_events_after_timestamp(const std::chrono::steady_clock::time_point& timestamp) const;

    // Remove events before a certain timestamp
    size_t remove_events_before_timestamp(const std::chrono::steady_clock::time_point& timestamp);

    // Get all events after a certain index
    std::vector<NaluEvent*> get_events_after_index(size_t index) const;

    // Remove events before a certain index
    size_t remove_events_before_index(size_t index);

private:
    std::vector<std::unique_ptr<NaluEvent>> events;  // Using unique_ptr for NaluEvent
    size_t max_events;  // Max number of events allowed
    std::function<void()> overflow_callback;  // Callback for overflow
};

#endif // NALU_EVENT_BUFFER_H
