/**
 * @file event_builder.h
 * @brief Event-building logic that groups packets into complete events.
 */

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "nalu_event_collector/collector/event_buffer.h"
#include "nalu_event_collector/config/event_builder_config.h"
#include "nalu_event_collector/data/packet.h"
#include "nalu_event_collector/timing/time_difference_calculator.h"

namespace nalu_event_collector {

/**
 * @brief Converts parsed packets into buffered event objects.
 *
 * `EventBuilder` applies trigger-window grouping rules, manages a short
 * post-trigger safety region, and forwards packet ownership into an
 * `EventBuffer`.
 */
class EventBuilder {
  public:
    /** @brief Construct an event builder from explicit grouping parameters. */
    EventBuilder(std::vector<int> channels,
                 int windows,
                 std::string trigger_type,
                 uint32_t time_threshold = 5000,
                 uint32_t max_trigger_time = 16777216,
                 size_t max_lookback = 2,
                 size_t event_max_size = 1024,
                 uint16_t event_header = 0xBBBB,
                 uint16_t event_trailer = 0xEEEE,
                 uint32_t clock_frequency = 23843000,
                 uint32_t event_completion_time_us = 10000);

    /** @brief Construct an event builder from a configuration object. */
    explicit EventBuilder(const EventBuilderConfig& config);

    /** @brief Process a parsed packet batch and update event state. */
    void collect_events(const std::vector<Packet>& packets);

    /** @brief Access the owned event buffer. */
    EventBuffer& get_event_buffer() { return event_buffer_; }

    /** @brief Return the configured number of windows per event. */
    int get_windows() const { return windows_; }

    /** @brief Return the configured active channel list. */
    const std::vector<int>& get_channels() const { return channels_; }

    /** @brief Return the configured trigger mode label. */
    const std::string& get_trigger_type() const { return trigger_type_; }

    /** @brief Return the trigger matching threshold expressed as a duration. */
    const std::chrono::steady_clock::duration& get_time_threshold_duration() const {
        return time_threshold_duration_;
    }

    /** @brief Override the size of the post-event safety region. */
    void set_post_event_safety_buffer_counter_max(size_t counter_max);

  private:
    void manage_safety_buffer();
    std::chrono::steady_clock::duration ticks_to_duration(uint32_t ticks, uint32_t clock_freq) const;

    std::vector<int> channels_;
    int windows_;
    std::string trigger_type_;
    std::chrono::steady_clock::duration time_threshold_duration_;
    size_t post_event_safety_buffer_counter_max_;
    size_t post_event_safety_buffer_counter_ = 0;
    bool in_safety_buffer_zone_ = false;
    uint32_t event_index_ = 0;
    TimeDifferenceCalculator time_diff_calculator_;
    EventBuffer event_buffer_;
};

}  // namespace nalu_event_collector
