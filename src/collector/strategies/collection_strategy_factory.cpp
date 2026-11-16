/**
 * @file collection_strategy_factory.cpp
 * @brief Implements construction of built-in collection strategies.
 */

#include "nalu_event_collector/collector/strategies/collection_strategy_factory.h"

#include <stdexcept>

#include "nalu_event_collector/collector/strategies/time_batch/time_batch_collection_strategy.h"
#include "nalu_event_collector/collector/strategies/trigger_time/trigger_time_collection_strategy.h"

namespace nalu_event_collector {

std::unique_ptr<CollectionStrategy> make_collection_strategy(
    const EventBuilderConfig& config) {
    if (config.settings.collection_strategy == "trigger_time") {
        return std::make_unique<TriggerTimeCollectionStrategy>(
            config, config.settings.collection_strategies.trigger_time);
    }
    if (config.settings.collection_strategy == "time_batch") {
        return std::make_unique<TimeBatchCollectionStrategy>(
            config, config.settings.collection_strategies.time_batch);
    }
    throw std::invalid_argument(
        "Unknown collection_strategy '" + config.settings.collection_strategy +
        "'. Supported strategies are 'trigger_time' and 'time_batch'.");
}

}  // namespace nalu_event_collector
