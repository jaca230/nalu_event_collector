/**
 * @file collection_strategies_config.h
 * @brief Registry of built-in collection-strategy configurations.
 */

#pragma once

#include "nalu_event_collector/collector/strategies/time_batch/time_batch_collection_strategy_config.h"
#include "nalu_event_collector/collector/strategies/trigger_time/trigger_time_collection_strategy_config.h"

namespace nalu_event_collector {

struct CollectionStrategiesConfig {
    TriggerTimeCollectionStrategyConfig trigger_time;
    TimeBatchCollectionStrategyConfig time_batch;
};

}  // namespace nalu_event_collector
