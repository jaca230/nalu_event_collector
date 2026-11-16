/**
 * @file event_builder_config.h
 * @brief Configuration for packet-to-event grouping behavior.
 */

#pragma once

#include "nalu_event_collector/config/event_builder_settings.h"
#include "nalu_event_collector/config/event_source_config.h"

namespace nalu_event_collector {

/**
 * @brief Configuration for constructing an EventBuilder.
 */
struct EventBuilderConfig {
    EventBuilderSettings settings;
    EventSourceConfig event_source;
};

}  // namespace nalu_event_collector
