/**
 * @file collection_strategy_factory.h
 * @brief Factory for built-in packet collection strategies.
 */

#pragma once

#include <memory>

#include "nalu_event_collector/collector/strategies/collection_strategy.h"
#include "nalu_event_collector/config/event_builder_config.h"

namespace nalu_event_collector {

/** @brief Construct the built-in strategy selected by @p config. */
std::unique_ptr<CollectionStrategy> make_collection_strategy(
    const EventBuilderConfig& config);

}  // namespace nalu_event_collector
