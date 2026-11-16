/**
 * @file event_builder.cpp
 * @brief Implements the collection-strategy facade.
 */

#include "nalu_event_collector/collector/event_builder.h"

#include <stdexcept>
#include <utility>

#include "nalu_event_collector/collector/strategies/collection_strategy_factory.h"

namespace nalu_event_collector {

namespace {

const TriggerTimeCollectionStrategyConfig& selected_trigger_config(
    const EventBuilderConfig& config) {
    if (config.settings.collection_strategy == "time_batch") {
        return config.settings.collection_strategies.time_batch.trigger_events;
    }
    return config.settings.collection_strategies.trigger_time;
}

}  // namespace

EventBuilder::EventBuilder(std::vector<int> channels, int windows,
                           std::string trigger_type, bool wlc_mode,
                           uint32_t time_threshold, uint32_t max_trigger_time,
                           size_t max_lookback, size_t event_max_size,
                           uint16_t event_header, uint16_t event_trailer,
                           uint32_t clock_frequency,
                           uint32_t event_completion_time_us)
    : EventBuilder([&]() {
          EventBuilderConfig config;
          auto& strategy = config.settings.collection_strategies.trigger_time;
          config.event_source.channels = std::move(channels);
          config.event_source.windows = windows;
          config.event_source.trigger_type = std::move(trigger_type);
          config.event_source.wlc_mode = wlc_mode;
          strategy.time_threshold = time_threshold;
          strategy.max_trigger_time = max_trigger_time;
          strategy.max_lookback = max_lookback;
          config.settings.max_events_in_buffer = event_max_size;
          config.settings.event_header = event_header;
          config.settings.event_trailer = event_trailer;
          strategy.clock_frequency = clock_frequency;
          strategy.event_completion_time_us = event_completion_time_us;
          return config;
      }()) {}

EventBuilder::EventBuilder(const EventBuilderConfig& config)
    : EventBuilder(config, make_collection_strategy(config)) {}

EventBuilder::EventBuilder(const EventBuilderConfig& config,
                           std::unique_ptr<CollectionStrategy> strategy)
    : config_(config),
      strategy_(std::move(strategy)),
      time_threshold_duration_(
          ticks_to_duration(selected_trigger_config(config).time_threshold,
                            selected_trigger_config(config).clock_frequency)) {
    if (strategy_ == nullptr) {
        throw std::invalid_argument("collection strategy cannot be null.");
    }
}

void EventBuilder::collect_events(const std::vector<Packet>& packets) {
    strategy_->collect(packets);
}

void EventBuilder::poll() { strategy_->poll(); }

void EventBuilder::flush() { strategy_->flush(); }

void EventBuilder::set_post_event_safety_buffer_counter_max(
    size_t counter_max) {
    static_cast<void>(counter_max);
}

const TriggerTimeCollectionStrategyConfig& EventBuilder::trigger_event_config()
    const {
    return selected_trigger_config(config_);
}

std::chrono::steady_clock::duration EventBuilder::ticks_to_duration(
    uint32_t ticks, uint32_t clock_freq) const {
    if (clock_freq == 0) {
        return std::chrono::steady_clock::duration::zero();
    }
    const double ns =
        static_cast<double>(ticks) * (1e9 / static_cast<double>(clock_freq));
    return std::chrono::duration_cast<std::chrono::steady_clock::duration>(
        std::chrono::nanoseconds(static_cast<int64_t>(ns)));
}

}  // namespace nalu_event_collector
