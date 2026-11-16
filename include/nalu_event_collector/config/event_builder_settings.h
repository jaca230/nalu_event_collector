/**
 * @file event_builder_settings.h
 * @brief Canonical stored settings for event collection strategies.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "nalu_event_collector/collector/strategies/collection_strategies_config.h"

namespace nalu_event_collector {

struct EventBuilderSettings {
    std::string collection_strategy = "trigger_time";
    CollectionStrategiesConfig collection_strategies;
    size_t max_events_in_buffer = 10000;
    uint16_t event_header = 0xBBBB;
    uint16_t event_trailer = 0xEEEE;
};

}  // namespace nalu_event_collector
