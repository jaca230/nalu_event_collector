#include "nalu_event_buffer.h"
#include <stdexcept>  // For std::out_of_range
#include <algorithm>  // For std::remove_if
#include <chrono>     // For timestamp

// Constructor
NaluEventBuffer::NaluEventBuffer(size_t max_events)
    : max_events(max_events) {}

// Destructor automatically handles cleanup for unique_ptr
NaluEventBuffer::~NaluEventBuffer() {
    // No need for manual deletion; unique_ptr handles memory management
}

// Add event using unique_ptr
void NaluEventBuffer::add_event(std::unique_ptr<NaluEvent> event) {
    if (events.size() >= max_events) {
        if (overflow_callback) {
            overflow_callback();
        }
        throw std::overflow_error("Buffer is full. Cannot add more events.");
    }
    events.push_back(std::move(event));  // Store unique_ptr
}

// Return a reference to the events
std::vector<std::unique_ptr<NaluEvent>>& NaluEventBuffer::get_events() {
    return events;
}

// Get the latest event (by reference)
NaluEvent& NaluEventBuffer::get_latest_event() {
    if (events.empty()) {
        throw std::out_of_range("No events in the buffer.");
    }
    return *events.back(); // Return reference
}

// Get an event by index (by reference)
NaluEvent& NaluEventBuffer::get_event_by_index(size_t index) {
    if (index >= events.size()) {
        throw std::out_of_range("Index is out of range.");
    }
    return *events[index]; // Return reference
}

// Set overflow callback
void NaluEventBuffer::set_on_overflow_callback(std::function<void()> callback) {
    overflow_callback = std::move(callback);
}

// Set max events and trim if necessary
void NaluEventBuffer::set_max_events(size_t new_max_events) {
    max_events = new_max_events;
    if (events.size() > max_events) {
        events.erase(events.begin(), events.begin() + (events.size() - max_events));
    }
}

// Remove events before a given timestamp
size_t NaluEventBuffer::remove_events_before_timestamp(const std::chrono::steady_clock::time_point& timestamp) {
    auto it = std::remove_if(events.begin(), events.end(),
                             [&](const std::unique_ptr<NaluEvent>& event) {
                                 return event->get_creation_timestamp() <= timestamp;
                             });

    size_t num_removed = std::distance(it, events.end());
    events.erase(it, events.end());

    return num_removed;
}

// Remove events before a given index
size_t NaluEventBuffer::remove_events_before_index(size_t index) {
    if (index >= events.size()) {
        throw std::out_of_range("Index is out of range.");
    }

    size_t num_removed = index;
    events.erase(events.begin(), events.begin() + index);
    return num_removed;
}

// Get all events after a certain timestamp
std::vector<NaluEvent*> NaluEventBuffer::get_events_after_timestamp(
    const std::chrono::steady_clock::time_point& timestamp) const {

    std::vector<NaluEvent*> result;
    for (const auto& event : events) {
        if (event->get_creation_timestamp() > timestamp) {
            result.push_back(event.get());  // Store raw pointer
        }
    }
    return result;
}

// Get all events after a certain index
std::vector<NaluEvent*> NaluEventBuffer::get_events_after_index(size_t index) const {
    std::vector<NaluEvent*> result;

    if (index < events.size()) {
        for (size_t i = index + 1; i < events.size(); ++i) {
            result.push_back(events[i].get());  // Store raw pointer
        }
    }

    return result;
}
