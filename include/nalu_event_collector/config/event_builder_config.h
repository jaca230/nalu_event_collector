/**
 * @file event_builder_config.h
 * @brief Configuration for packet-to-event grouping behavior.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include "nalu_event_collector/collector/strategies/collection_strategies_config.h"

namespace nalu_event_collector {

/**
 * @brief Configuration for constructing an EventBuilder.
 */
struct EventBuilderConfig {
    /**
     * @brief Packet collection strategy.
     *
     * `trigger_time` preserves the traditional one-output-event-per-trigger
     * behavior. `time_batch` first forms those trigger events and then combines
     * them into larger wall-clock batches.
     */
    std::string collection_strategy = "trigger_time";

    /** @brief Strategy-owned configuration, keyed by strategy name. */
    CollectionStrategiesConfig collection_strategies;

    /** @brief Maximum number of retained events in the rolling event buffer. */
    size_t max_events_in_buffer = 10000;

    /** @brief Event header word written into serialized events. */
    uint16_t event_header = 0xBBBB;

    /** @brief Event trailer word written into serialized events. */
    uint16_t event_trailer = 0xEEEE;
};

}  // namespace nalu_event_collector
