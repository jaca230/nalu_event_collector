#ifndef NALU_EVENT_BUILDER_H
#define NALU_EVENT_BUILDER_H

#include <vector>
#include "nalu_packet.h"
#include "nalu_event_buffer.h"
#include "nalu_time_difference_calculator.h"
#include "nalu_event_collector_params.h"

class NaluEventBuilder {
public:
    // Constructor
    NaluEventBuilder(std::vector<int> channels, int windows,
                     int time_threshold = 5000, uint32_t max_trigger_time = 16777216, 
                     size_t max_lookback = 2, size_t event_max_size = 1024, 
                     uint16_t event_header = 0xBBBB, uint16_t event_trailer = 0xEEEE);

    NaluEventBuilder(const NaluEventCollectorParams& params);

    // Method to collect events from packets
    void collect_events(const std::vector<NaluPacket>& packets);

    // Getter for event buffer
    NaluEventBuffer& get_event_buffer() { return event_buffer; }

    // Getter for windows
    int get_windows() const { return windows; }

    // Getter for channels
    const std::vector<int>& get_channels() const { return channels; }

    // Setter to adjust the post_event_safety_buffer_counter_max
    void set_post_event_safety_buffer_counter_max(size_t counter_max);

private:
    // Member variables
    std::vector<int> channels;  // Channel data
    int windows;  // Window size

    size_t post_event_safety_buffer_counter_max;  // Max counter for safety buffer
    size_t post_event_safety_buffer_counter = 0;  // Current counter
    bool in_safety_buffer_zone = false;  // Safety buffer zone flag

    uint32_t event_index = 0; //Event index tracker

    // Event buffer to manage the events internally
    NaluEventBuffer event_buffer;

    // Time difference calculator
    NaluTimeDifferenceCalculator time_diff_calculator;

    // Helper function to manage the safety buffer
    void manage_safety_buffer();
};

#endif // NALU_EVENT_BUILDER_H
