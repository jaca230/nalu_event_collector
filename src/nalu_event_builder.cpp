#include "nalu_event_builder.h"

#include <cmath>

// Constructor
NaluEventBuilder::NaluEventBuilder(std::vector<int> channels, int windows,
                                   std::string trigger_type,
                                   int time_threshold /*= 5000*/,
                                   uint32_t max_trigger_time /*= 16777216*/,
                                   size_t max_lookback /*= 2*/,
                                   size_t event_buffer_max_size /*= 1024*/,
                                   uint16_t event_header /*= 0xBBBB*/,
                                   uint16_t event_trailer /*= 0xEEEE*/,
                                   uint32_t clock_frequency /*= 23843000*/)
    : channels(std::move(channels)),
      windows(windows),
      trigger_type(trigger_type),
      time_diff_calculator(max_trigger_time, time_threshold),
      event_buffer(event_buffer_max_size, time_diff_calculator, max_lookback,
                   this->channels, static_cast<uint8_t>(windows),
                   event_header, event_trailer, trigger_type) {
    // Calculate the post-event safety buffer size as 10% of the event size (rounded up)
    post_event_safety_buffer_counter_max =
        std::ceil(this->channels.size() * windows * 0.10);
    
    time_threshold_duration = ticks_to_duration(static_cast<uint32_t>(time_threshold), clock_frequency);
    
}


NaluEventBuilder::NaluEventBuilder(const NaluEventBuilderParams& params)
    : NaluEventBuilder(params.channels,
                       params.windows,
                       params.trigger_type,
                       params.time_threshold,
                       params.max_trigger_time,
                       params.max_lookback,
                       params.max_events_in_buffer,
                       params.event_header,
                       params.event_trailer,
                       params.clock_frequency) {}


// Setter to adjust the post_event_safety_buffer_counter_max
void NaluEventBuilder::set_post_event_safety_buffer_counter_max(
    size_t counter_max) {
    post_event_safety_buffer_counter_max = counter_max;
}
// Method to collect events from packets
void NaluEventBuilder::collect_events(const std::vector<NaluPacket>& packets) {
    for (const auto& packet : packets) {
        // Add packet to the event buffer and manage the safety buffer
        event_buffer.add_packet(packet, in_safety_buffer_zone, event_index);
        manage_safety_buffer();
    }
}


void NaluEventBuilder::manage_safety_buffer() {
    if (in_safety_buffer_zone) {
        post_event_safety_buffer_counter++;
        if (post_event_safety_buffer_counter >=
            post_event_safety_buffer_counter_max) {
            in_safety_buffer_zone = false;
            post_event_safety_buffer_counter = 0;
        }
    }
}

std::chrono::steady_clock::duration NaluEventBuilder::ticks_to_duration(uint32_t ticks, uint32_t clock_freq) const {
    if (clock_freq == 0) return std::chrono::steady_clock::duration::zero();
    double ns = static_cast<double>(ticks) * (1e9 / static_cast<double>(clock_freq));
    return std::chrono::duration_cast<std::chrono::steady_clock::duration>(
        std::chrono::nanoseconds(static_cast<int64_t>(ns)));
}
