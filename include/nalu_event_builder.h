#ifndef NALU_EVENT_BUILDER_H
#define NALU_EVENT_BUILDER_H

#include <vector>
#include "nalu_packet.h"
#include "nalu_event_buffer.h"

class NaluEventBuilder {
public:
    // Constructor
    NaluEventBuilder(std::vector<int> channels, int windows, 
                     int time_threshold = 5000, uint32_t max_trigger_time = 16777216 /* 2^24 clock ticks*/);

    // Method to collect events from packets
    void collect_events(const std::vector<NaluPacket>& packets);

    // Setter to adjust the post_event_safety_buffer_size
    void set_post_event_safety_buffer_counter_max(size_t counter_max);

    // Setter to adjust max_lookback
    void set_max_lookback(size_t max_lookback) { this->max_lookback = max_lookback; }

    // Get the event buffer directly
    NaluEventBuffer& get_event_buffer() { return event_buffer; }

private:
    // Helper methods
    void add_packet_to_event(const NaluPacket& packet);
    int find_event_index(uint32_t trigger_time);
    inline uint32_t compute_time_diff(uint32_t new_time, uint32_t old_time);
    void manage_safety_buffer();

    // Member variables
    int time_threshold_between_events;  // Time threshold for event separation
    int windows;  // Window size
    std::vector<int> channels;  // Channel data
    uint32_t max_trigger_time;  // Maximum trigger time
    uint32_t half_max_trigger_time; //Half the max trigger time, to avoid computing many times

    size_t post_event_safety_buffer_counter_max;  // Max counter for safety buffer
    size_t post_event_safety_buffer_counter = 0;  // Current counter
    bool in_safety_buffer_zone = false;  // Safety buffer zone flag
    size_t max_lookback = 2;  // Maximum number of events to look back when in safety buffer
    
    size_t event_max_packets;  // Max packets in an event
    size_t event_index = 0;  // Event index tracker

    // Event buffer to manage the events internally
    NaluEventBuffer event_buffer;
};

#endif // NALU_EVENT_BUILDER_H
