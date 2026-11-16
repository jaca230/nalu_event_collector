/**
 * @file trigger_time_collection_strategy_config.h
 * @brief Configuration owned by trigger-time event collection.
 */

#pragma once

#include <cstddef>
#include <cstdint>

namespace nalu_event_collector {

struct TriggerTimeCollectionStrategyConfig {
    uint32_t time_threshold = 5000;
    uint32_t event_completion_time_us = 10000;
    uint32_t max_trigger_time = 16777216;
    uint32_t clock_frequency = 23843000;
    size_t max_lookback = 2;
};

}  // namespace nalu_event_collector
