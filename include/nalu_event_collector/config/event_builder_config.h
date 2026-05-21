/**
 * @file event_builder_config.h
 * @brief Configuration for packet-to-event grouping behavior.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace nalu_event_collector {

/**
 * @brief Return the default 32-channel HDSOC-style channel list.
 */
inline std::vector<int> default_channels() {
    std::vector<int> channels;
    channels.reserve(32);
    for (int channel = 0; channel < 32; ++channel) {
        channels.push_back(channel);
    }
    return channels;
}

/**
 * @brief Configuration for constructing an EventBuilder.
 */
struct EventBuilderConfig {
    /** @brief Channel identifiers expected in each event. */
    std::vector<int> channels = default_channels();

    /** @brief Trigger mode label, typically `self`, `ext`, or `imm`. */
    std::string trigger_type = "self";

    /** @brief Number of windows expected per channel for complete events. */
    int windows = 4;

    /** @brief Trigger-time threshold in device ticks for packet matching. */
    uint32_t time_threshold = 5000;

    /** @brief Completion timeout for self-trigger events in microseconds. */
    uint32_t event_completion_time_us = 10000;

    /** @brief Maximum number of retained events in the rolling event buffer. */
    size_t max_events_in_buffer = 10000;

    /** @brief Maximum trigger-time counter before wraparound. */
    uint32_t max_trigger_time = 16777216;

    /** @brief Device clock frequency used for duration conversion. */
    uint32_t clock_frequency = 23843000;

    /** @brief Number of prior events to search while in the safety region. */
    size_t max_lookback = 2;

    /** @brief Event header word written into serialized events. */
    uint16_t event_header = 0xBBBB;

    /** @brief Event trailer word written into serialized events. */
    uint16_t event_trailer = 0xEEEE;
};

}  // namespace nalu_event_collector
