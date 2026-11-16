/**
 * @file time_batch_collection_strategy_config.h
 * @brief Configuration owned by fixed-interval time batching.
 */

#pragma once

#include <cstddef>
#include <cstdint>

#include "nalu_event_collector/collector/strategies/trigger_time/trigger_time_collection_strategy_config.h"

namespace nalu_event_collector {

struct TimeBatchCollectionStrategyConfig {
    uint32_t duration_us = 100000;
    size_t max_packets = 65535;
    TriggerTimeCollectionStrategyConfig trigger_events;
};

}  // namespace nalu_event_collector
