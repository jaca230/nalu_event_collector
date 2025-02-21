#include "nalu_event_builder.h"
#include <cmath>  
#include <algorithm>  // For std::remove_if
#include <limits>

// Constructor
NaluEventBuilder::NaluEventBuilder(int time_threshold, int windows, 
                                   std::vector<int> channels, uint32_t max_trigger_time)
    : time_threshold_between_events(time_threshold), windows(windows), 
      channels(std::move(channels)), max_trigger_time(max_trigger_time) {
    
    // Pre-allocate about 10 entries for events
    events.reserve(10);

    // Calculate the post-event safety buffer size as 10% of the event size (rounded up)
    post_event_safety_buffer_counter_max = std::ceil(channels.size() * windows * 0.10);

    event_max_packets = channels.size() * windows + 5; //Expected number of packets plus small buffer
}

// Setter to adjust the post_event_safety_buffer_size
void NaluEventBuilder::set_post_event_safety_buffer_counter_max(size_t counter_max) {
    post_event_safety_buffer_counter_max = counter_max;
}

// Method to clean events and keep only events starting from a given index
void NaluEventBuilder::clean_events(size_t start_index) {
    // Ensure the start_index is within the valid range
    if (start_index < events.size()) {
        // Erase all events before the specified index
        events.erase(events.begin(), events.begin() + start_index);
    }
}

// Method to find all events with a newer timestamp by iterating forwards
std::pair<std::vector<NaluEventWrapper>, size_t> NaluEventBuilder::get_events_after_timestamp(uint32_t timestamp) {
    std::vector<NaluEventWrapper> events_after_timestamp;
    size_t oldest_event_index = -1;

    // Store the casted max_trigger_time once for efficiency
    int32_t max_trigger_time_signed = static_cast<int32_t>(max_trigger_time);

    // Iterate forwards through the events to find events after the timestamp
    for (size_t i = 0; i < events.size(); ++i) {
        NaluEvent& event = events[i].get_event();

        // Compute the signed time difference, casting exactly once
        int32_t diff = static_cast<int32_t>(event.reference_time) - static_cast<int32_t>(timestamp);

        // If the difference is too large (wraparound case), adjust the diff
        if (diff > max_trigger_time_signed / 2) {
            diff -= max_trigger_time_signed;
        } else if (diff < -max_trigger_time_signed / 2) {
            diff += max_trigger_time_signed;
        }

        // Only consider events with a newer timestamp (after wraparound adjustment)
        if (diff > 0) {
            // If it's the first event we're considering, track its index as the oldest
            if (oldest_event_index == -1) {
                oldest_event_index = i;
            }

            // Add this event to the list of events after the timestamp
            events_after_timestamp.push_back(events[i]);
        }
    }

    // Return the vector of events after the timestamp and the index of the oldest one
    return {events_after_timestamp, oldest_event_index};
}


// Updated collect_events method to handle the "n" packets after event creation
void NaluEventBuilder::collect_events(const std::vector<NaluPacket>& packets) {
    for (const auto& packet : packets) {
        add_packet_to_event(packet);
        manage_safety_buffer();
    }
}


// Updated add_packet_to_event method to handle the new "n" logic
void NaluEventBuilder::add_packet_to_event(const NaluPacket& packet) {
    uint32_t trigger_time = packet.trigger_time;

    // Find the appropriate event index
    int matched_event_index = find_event_index(trigger_time);

    // If no suitable match is found, create a new event
    if (matched_event_index == -1) {
        events.emplace_back(0, 0, this->event_index++, trigger_time, 0, 0, 0, event_max_packets);
        post_event_safety_buffer_counter = 0;
        in_safety_buffer_zone = true;  
        matched_event_index = events.size() - 1;
    }

    // Add packet to the matched event
    events[matched_event_index].add_packet(packet);
}


int NaluEventBuilder::find_event_index(uint32_t trigger_time) {
    int matched_event_index = -1;

    // Check the most recent event first
    uint32_t time_diff = compute_time_diff(trigger_time, events.back().get_event().reference_time);
    if (time_diff <= time_threshold_between_events) {
        matched_event_index = events.size() - 1;
    } 
    // If in safety buffer zone and no match was found, check the second most recent event
    else if (in_safety_buffer_zone && events.size() > 1) {
        time_diff = compute_time_diff(trigger_time, events[events.size() - 2].get_event().reference_time);
        if (time_diff <= time_threshold_between_events) {
            matched_event_index = events.size() - 2;
        }
    }

    return matched_event_index;
}

inline uint32_t NaluEventBuilder::compute_time_diff(uint32_t new_time, uint32_t old_time) {
    uint32_t abs_diff = (new_time >= old_time) ? (new_time - old_time) : (old_time - new_time);
    
    if (abs_diff > (max_trigger_time / 2)) {
        return max_trigger_time - abs_diff + 1;
    }
    
    return abs_diff;
}

void NaluEventBuilder::manage_safety_buffer() {
    if (in_safety_buffer_zone) {
        post_event_safety_buffer_counter++;
        if (post_event_safety_buffer_counter >= post_event_safety_buffer_counter_max) {
            in_safety_buffer_zone = false;
            post_event_safety_buffer_counter = 0;
        }
    }
}

// Get collected events
const std::vector<NaluEventWrapper>& NaluEventBuilder::get_events() const {
    return events;
}
