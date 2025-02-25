#include "nalu_event_builder.h"
#include <cmath>
#include <algorithm>  // For std::remove_if
#include <limits>
#include <iostream>

// Constructor
NaluEventBuilder::NaluEventBuilder(std::vector<int> channels, int windows, 
                                   int time_threshold /*= 5000*/, uint32_t max_trigger_time /*= 16777216*/)
    : time_threshold_between_events(time_threshold), windows(windows), 
      channels(std::move(channels)), max_trigger_time(max_trigger_time),
      event_buffer() {

    // Store half of the max_trigger_time to avoid recomputing
    half_max_trigger_time = max_trigger_time / 2;
    
    // Calculate the post-event safety buffer size as 10% of the event size (rounded up)
    post_event_safety_buffer_counter_max = std::ceil(channels.size() * windows * 0.10);
    event_max_packets = this->channels.size() * this->windows + 5; // Expected number of packets plus small buffer
}


// Setter to adjust the post_event_safety_buffer_size
void NaluEventBuilder::set_post_event_safety_buffer_counter_max(size_t counter_max) {
    post_event_safety_buffer_counter_max = counter_max;
}

// Method to collect events from packets
void NaluEventBuilder::collect_events(const std::vector<NaluPacket>& packets) {
    for (const auto& packet : packets) {
        add_packet_to_event(packet);
        manage_safety_buffer();
    }
}

void NaluEventBuilder::add_packet_to_event(const NaluPacket& packet) {
    uint32_t trigger_time = packet.trigger_time;

    // Find the appropriate event index using the event buffer
    int matched_event_index = find_event_index(trigger_time);

    // If no suitable match is found, create a new event
    if (matched_event_index == -1) {
        std::unique_ptr<NaluEvent> new_event = std::make_unique<NaluEvent>(
            0xBBBB,  // Set header to 0xBBBB (hexadecimal)
            0,       // info
            this->event_index++, 
            trigger_time, 
            0,       // packet_size
            0,       // num_packets
            0xEEEE,  // Set footer to 0xEEEE (hexadecimal)
            event_max_packets
        );
        // Add unique pointer to the event buffer
        event_buffer.add_event(std::move(new_event));  // Move ownership of the unique pointer
        post_event_safety_buffer_counter = 0;
        in_safety_buffer_zone = true;
        matched_event_index = event_buffer.get_events().size() - 1;  // Event will be added after this point
    }

    // Add packet to the matched event in the buffer
    event_buffer.get_event_by_index(matched_event_index).add_packet(packet);
}

int NaluEventBuilder::find_event_index(uint32_t trigger_time) {
    int matched_event_index = -1;

    // Get the vector of events in the buffer (avoid copying)
    std::vector<std::unique_ptr<NaluEvent>>& events = event_buffer.get_events();
    size_t events_size = events.size();

    if (events_size > 0) {
        // Calculate the number of events to check based on the safety buffer zone
        size_t lookback_limit = in_safety_buffer_zone ? std::min(max_lookback, events_size) : 1;

        for (size_t i = 0; i < lookback_limit; ++i) {
            uint32_t time_diff = compute_time_diff(trigger_time, events[events_size - 1 - i]->reference_time);
            if (time_diff <= time_threshold_between_events) {
                matched_event_index = events_size - 1 - i;
                break;
            }
        }
    }

    return matched_event_index;
}

inline uint32_t NaluEventBuilder::compute_time_diff(uint32_t new_time, uint32_t old_time) {
    uint32_t abs_diff = (new_time >= old_time) ? (new_time - old_time) : (old_time - new_time);
    
    if (abs_diff > half_max_trigger_time) {
        return max_trigger_time - abs_diff;
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
