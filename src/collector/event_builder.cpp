/**
 * @file event_builder.cpp
 * @brief Implements packet grouping and safety-buffer handling for events.
 */

#include "nalu_event_collector/collector/event_builder.h"

#include <cmath>

namespace nalu_event_collector {

EventBuilder::EventBuilder(std::vector<int> channels,
                           int windows,
                           std::string trigger_type,
                           bool wlc_mode,
                           uint32_t time_threshold,
                           uint32_t max_trigger_time,
                           size_t max_lookback,
                           size_t event_max_size,
                           uint16_t event_header,
                           uint16_t event_trailer,
                           uint32_t clock_frequency,
                           uint32_t event_completion_time_us)
    : channels_(std::move(channels)),
      windows_(windows),
      trigger_type_(std::move(trigger_type)),
      time_threshold_duration_(ticks_to_duration(time_threshold, clock_frequency)),
      post_event_safety_buffer_counter_max_(0),
      time_diff_calculator_(max_trigger_time, time_threshold),
      event_buffer_(event_max_size,
                    time_diff_calculator_,
                    max_lookback,
                    channels_,
                    static_cast<uint8_t>(windows_),
                    event_header,
                    event_trailer,
                    trigger_type_,
                    wlc_mode,
                    time_threshold,
                    clock_frequency,
                    event_completion_time_us) {
    post_event_safety_buffer_counter_max_ =
        static_cast<size_t>(std::ceil(channels_.size() * windows_ * 0.10));
}

EventBuilder::EventBuilder(const EventBuilderConfig& config)
    : EventBuilder(config.channels,
                   config.windows,
                   config.trigger_type,
                   config.wlc_mode,
                   config.time_threshold,
                   config.max_trigger_time,
                   config.max_lookback,
                   config.max_events_in_buffer,
                   config.event_header,
                   config.event_trailer,
                   config.clock_frequency,
                   config.event_completion_time_us) {}

void EventBuilder::set_post_event_safety_buffer_counter_max(size_t counter_max) {
    post_event_safety_buffer_counter_max_ = counter_max;
}

void EventBuilder::collect_events(const std::vector<Packet>& packets) {
    for (const auto& packet : packets) {
        event_buffer_.add_packet(packet, in_safety_buffer_zone_, event_index_);
        manage_safety_buffer();
    }
}

void EventBuilder::manage_safety_buffer() {
    if (!in_safety_buffer_zone_) {
        return;
    }

    ++post_event_safety_buffer_counter_;
    if (post_event_safety_buffer_counter_ >= post_event_safety_buffer_counter_max_) {
        in_safety_buffer_zone_ = false;
        post_event_safety_buffer_counter_ = 0;
    }
}

std::chrono::steady_clock::duration EventBuilder::ticks_to_duration(uint32_t ticks,
                                                                    uint32_t clock_freq) const {
    if (clock_freq == 0) {
        return std::chrono::steady_clock::duration::zero();
    }
    const double ns = static_cast<double>(ticks) * (1e9 / static_cast<double>(clock_freq));
    return std::chrono::duration_cast<std::chrono::steady_clock::duration>(
        std::chrono::nanoseconds(static_cast<int64_t>(ns)));
}

}  // namespace nalu_event_collector
