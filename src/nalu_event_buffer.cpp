#include "nalu_event_buffer.h"

#include <algorithm>
#include <stdexcept>

#include "nalu_event_collector_logger.h"

// Constructor
NaluEventBuffer::NaluEventBuffer(size_t max_events,
                                 NaluTimeDifferenceCalculator& time_diff_calculator,
                                 size_t max_lookback,
                                 const std::vector<int>& channels,
                                 uint8_t windows,
                                 uint16_t event_header,
                                 uint16_t event_trailer,
                                 std::string trigger_type)  
    : max_events(max_events),
      time_diff_calculator(time_diff_calculator),
      max_lookback(max_lookback),
      windows(windows),
      event_header(event_header),
      event_trailer(event_trailer)
{
    // Calculate the channel mask based on the channels vector.
    // Set channel_mask to 64-bit value.
    channel_mask = 0;
    // Set only the bits corresponding to channel numbers in the array
    for (int channel : channels) {
        if (channel >= 0 && channel < 64) { // Ensure channel is within valid range
            channel_mask |= (1ULL << channel);
        }
    }

    // Determine the "extra info" byte based on the trigger type
    // I'd use enums, but it's hardcoded like this in naludaq anyways so
    // adding enums would kind of be like beating a dead horse.
    uint8_t trigger_bits = 0;

    if (trigger_type == "ext") {
        trigger_bits = 0x1; // 01 binary
    } else if (trigger_type == "self") {
        trigger_bits = 0x2; // 10 binary
    } else if (trigger_type == "imm") {
        trigger_bits = 0x3; // 11 binary
    } else {
        trigger_bits = 0x0; // 00 binary
    }

    // Shift left by 4 to set bits 4 and 5 (zero-based)
    extra_info = (trigger_bits << 4); // This leaves 4 bits for "error codes"

    // Calculate max_event_size based on the number of channels and windows
    max_event_size = (windows * channels.size()) + 5;  // 5 is the additional overhead for event headers/footers
}

// Destructor
NaluEventBuffer::~NaluEventBuffer() {}

// Add event (thread-safe)
void NaluEventBuffer::add_event(std::unique_ptr<NaluEvent> event) {
    std::lock_guard<std::mutex> lock(buffer_mutex);
    add_event_helper(event);
}

// Get events (thread-safe)
std::vector<std::unique_ptr<NaluEvent>>& NaluEventBuffer::get_events() {
    std::lock_guard<std::mutex> lock(buffer_mutex);
    return events;
}

// Get latest event (thread-safe)
NaluEvent& NaluEventBuffer::get_latest_event() {
    std::lock_guard<std::mutex> lock(buffer_mutex);
    if (events.empty()) {
        NaluEventCollectorLogger::error(
            "Attempted to access latest event, but buffer is empty.");
        throw std::out_of_range("No events in the buffer.");
    }
    return *events.back();
}

// Get event by index (thread-safe)
NaluEvent& NaluEventBuffer::get_event_by_index(size_t index) {
    std::lock_guard<std::mutex> lock(buffer_mutex);
    if (index >= events.size()) {
        NaluEventCollectorLogger::error(
            "Attempted to access event at index " + std::to_string(index) +
            ", but buffer size is " + std::to_string(events.size()) + ".");
        throw std::out_of_range("Index is out of range.");
    }
    return *events[index];
}

// Set overflow callback (thread-safe)
void NaluEventBuffer::set_on_overflow_callback(std::function<void()> callback) {
    std::lock_guard<std::mutex> lock(buffer_mutex);
    overflow_callback = std::move(callback);
}

// Set max events and trim (thread-safe)
void NaluEventBuffer::set_max_events(size_t new_max_events) {
    std::lock_guard<std::mutex> lock(buffer_mutex);
    max_events = new_max_events;
    if (events.size() > max_events) {
        NaluEventCollectorLogger::warning(
            "Reducing max_events caused trimming of " +
            std::to_string(events.size() - max_events) + " old events.");
        events.erase(events.begin(),
                     events.begin() + (events.size() - max_events));
    }
}

// Remove events before a timestamp (thread-safe)
size_t NaluEventBuffer::remove_events_before_timestamp(
    const std::chrono::steady_clock::time_point& timestamp,
    ssize_t seed_index) {
    std::lock_guard<std::mutex> lock(buffer_mutex);

    if (seed_index < 0 || static_cast<size_t>(seed_index) >= events.size()) {
        seed_index = 0;
    }

    auto it =
        std::lower_bound(events.begin() + seed_index, events.end(), timestamp,
                         [](const std::unique_ptr<NaluEvent>& event,
                            const std::chrono::steady_clock::time_point& ts) {
                             return event->get_creation_timestamp() < ts;
                         });

    size_t num_removed = std::distance(events.begin(), it);
    events.erase(events.begin(), it);

    return num_removed;
}

// Remove events before an index (thread-safe)
size_t NaluEventBuffer::remove_events_before_index_exclusive(size_t index) {
    std::lock_guard<std::mutex> lock(buffer_mutex);
    if (index > events.size()) {
        NaluEventCollectorLogger::error(
            "Attempted to remove events before index " + std::to_string(index) +
            ", but buffer size is " + std::to_string(events.size()) + ".");
        throw std::out_of_range("Index is out of range.");
    }

    size_t num_removed = index;
    events.erase(events.begin(), events.begin() + num_removed);

    return num_removed;
}

// Get events after timestamp (thread-safe)
std::vector<NaluEvent*> NaluEventBuffer::get_events_after_timestamp(
    const std::chrono::steady_clock::time_point& timestamp,
    ssize_t seed_index) const {
    std::lock_guard<std::mutex> lock(buffer_mutex);
    std::vector<NaluEvent*> result;

    if (seed_index < 0 || static_cast<size_t>(seed_index) >= events.size()) {
        seed_index = 0;
    }

    auto it =
        std::lower_bound(events.begin() + seed_index, events.end(), timestamp,
                         [](const std::unique_ptr<NaluEvent>& event,
                            const std::chrono::steady_clock::time_point& ts) {
                             return event->get_creation_timestamp() < ts;
                         });

    for (; it != events.end(); ++it) {
        result.push_back(it->get());
    }

    return result;
}

// Get events after index (thread-safe)
std::vector<NaluEvent*> NaluEventBuffer::get_events_after_index_inclusive(
    size_t index) const {
    std::lock_guard<std::mutex> lock(buffer_mutex);
    std::vector<NaluEvent*> result;

    if (index >= events.size()) {
        return {};  // Return empty if no events after the given index
    }

    for (size_t i = index; i < events.size(); ++i) {
        result.push_back(events[i].get());
    }

    return result;
}

// Add packet to event buffer (thread-safe)
void NaluEventBuffer::add_packet(const NaluPacket& packet,
                                 bool& in_safety_buffer_zone,
                                 uint32_t& event_index) {
    std::lock_guard<std::mutex> lock(buffer_mutex);

    uint32_t trigger_time = packet.trigger_time;
    NaluEvent* matched_event = nullptr;

    size_t events_size = events.size();
    if (events_size > 0) {
        size_t lookback_limit =
            in_safety_buffer_zone ? std::min(max_lookback, events_size) : 1;

        for (size_t i = 0; i < lookback_limit; ++i) {
            if (time_diff_calculator.is_within_threshold(
                    trigger_time,
                    events[events_size - 1 - i]->header.reference_time)) {
                matched_event = events[events_size - 1 - i].get();
                break;
            }
        }
    }

    if (!matched_event) {
        auto new_event = std::make_unique<NaluEvent>(
            event_header, extra_info, event_index++, trigger_time, packet.get_size(), 0,
            event_trailer, max_event_size, channel_mask, windows);
        new_event->add_packet(packet);
        add_event_helper(new_event);
        in_safety_buffer_zone = true;
    } else {
        matched_event->add_packet(packet);
    }
}

void NaluEventBuffer::add_event_helper(std::unique_ptr<NaluEvent>& event) {
    if (events.size() >= max_events) {
        if (overflow_callback) {
            overflow_callback();
        }
        NaluEventCollectorLogger::error(
            "Buffer overflow! Max events reached: " +
            std::to_string(max_events) + ". Throwing exception.");
        throw std::overflow_error("Buffer is full. Cannot add more events.");
    }

    events.push_back(std::move(event));
}
