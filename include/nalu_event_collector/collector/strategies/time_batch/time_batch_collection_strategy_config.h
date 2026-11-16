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
    TriggerTimeCollectionStrategyConfig trigger_events = [] {
        TriggerTimeCollectionStrategyConfig config;
        // A batch deliberately stays open much longer than one trigger event.
        // Give an incomplete final trigger enough time to finish before the
        // batch is emitted under normal live-readout jitter.
        config.event_completion_time_us = 100000;
        return config;
    }();
};

}  // namespace nalu_event_collector
