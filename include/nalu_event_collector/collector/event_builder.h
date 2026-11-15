/**
 * @file event_builder.h
 * @brief Event-building logic that groups packets into complete events.
 */

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "nalu_event_collector/collector/strategies/collection_strategy.h"
#include "nalu_event_collector/config/event_builder_config.h"
#include "nalu_event_collector/data/packet.h"
#include "nalu_event_collector/timing/time_difference_calculator.h"

namespace nalu_event_collector {

/**
 * @brief Converts parsed packets into buffered event objects.
 *
 * `EventBuilder` is a compatibility-preserving facade over a pluggable
 * `CollectionStrategy`.
 */
class EventBuilder {
   public:
    /** @brief Construct an event builder from explicit grouping parameters. */
    EventBuilder(std::vector<int> channels, int windows,
                 std::string trigger_type, bool wlc_mode = false,
                 uint32_t time_threshold = 5000,
                 uint32_t max_trigger_time = 16777216, size_t max_lookback = 2,
                 size_t event_max_size = 1024, uint16_t event_header = 0xBBBB,
                 uint16_t event_trailer = 0xEEEE,
                 uint32_t clock_frequency = 23843000,
                 uint32_t event_completion_time_us = 10000);

    /** @brief Construct an event builder from a configuration object. */
    explicit EventBuilder(const EventBuilderConfig& config);

    /** @brief Construct an event builder with an application-defined strategy.
     */
    EventBuilder(const EventBuilderConfig& config,
                 std::unique_ptr<CollectionStrategy> strategy);

    /** @brief Process a parsed packet batch and update event state. */
    void collect_events(const std::vector<Packet>& packets);

    /** @brief Advance time-dependent strategy state without adding packets. */
    void poll();

    /** @brief Publish valid strategy-owned buffered output during shutdown. */
    void flush();

    /** @brief Access the owned event buffer. */
    EventBuffer& get_event_buffer() { return strategy_->event_buffer(); }

    /** @brief Access the active strategy. */
    CollectionStrategy& get_collection_strategy() { return *strategy_; }

    /** @brief Return the configured number of windows per event. */
    int get_windows() const { return trigger_event_config().windows; }

    /** @brief Return the configured active channel list. */
    const std::vector<int>& get_channels() const {
        return trigger_event_config().channels;
    }

    /** @brief Return the configured trigger mode label. */
    const std::string& get_trigger_type() const {
        return trigger_event_config().trigger_type;
    }

    /** @brief Return the trigger matching threshold expressed as a duration. */
    const std::chrono::steady_clock::duration& get_time_threshold_duration()
        const {
        return time_threshold_duration_;
    }

    /**
     * @brief Retained source-compatible no-op for the removed safety-buffer
     * heuristic.
     *
     * Complete channel groups are now searched against the configured event
     * lookback on every insertion, so a separate safety-region counter is no
     * longer used.
     */
    void set_post_event_safety_buffer_counter_max(size_t counter_max);

   private:
    const TriggerTimeCollectionStrategyConfig& trigger_event_config() const;
    std::chrono::steady_clock::duration ticks_to_duration(
        uint32_t ticks, uint32_t clock_freq) const;

    EventBuilderConfig config_;
    std::unique_ptr<CollectionStrategy> strategy_;
    std::chrono::steady_clock::duration time_threshold_duration_;
};

}  // namespace nalu_event_collector
