/**
 * @file time_batch_collection_strategy.h
 * @brief Fixed-interval aggregate event collection strategy.
 */

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>

#include "nalu_event_collector/collector/strategies/collection_strategy.h"
#include "nalu_event_collector/collector/strategies/time_batch/time_batch_collection_strategy_config.h"
#include "nalu_event_collector/collector/strategies/trigger_time/trigger_time_collection_strategy.h"
#include "nalu_event_collector/config/event_builder_config.h"

namespace nalu_event_collector {

/**
 * @brief Combines complete trigger events into fixed wall-clock batches.
 *
 * The nested trigger-time strategy performs the normal self/external trigger
 * association first. Packets from those complete trigger events are then
 * appended to one larger output event. Original packet trigger timestamps are
 * preserved, so downstream consumers can recover the constituent triggers.
 */
class TimeBatchCollectionStrategy final : public CollectionStrategy {
   public:
    TimeBatchCollectionStrategy(
        const EventBuilderConfig& config,
        const TimeBatchCollectionStrategyConfig& strategy_config);

    void collect(const std::vector<Packet>& packets) override;
    void poll() override;
    void flush() override;
    EventBuffer& event_buffer() override { return output_buffer_; }

   private:
    void drain_trigger_events(bool finalize_complete_groups);
    void append_trigger_event(const Event& event);
    void advance_deadline(std::chrono::steady_clock::time_point now);
    void finalize_batch();

    EventBuilderConfig config_;
    TimeBatchCollectionStrategyConfig strategy_config_;
    TriggerTimeCollectionStrategy trigger_strategy_;
    EventBuffer output_buffer_;
    std::unique_ptr<Event> current_batch_;
    std::chrono::microseconds batch_duration_;
    uint16_t max_packets_per_batch_;
    uint32_t batch_index_ = 0;
    bool deadline_initialized_ = false;
    std::chrono::steady_clock::time_point next_deadline_;
};

}  // namespace nalu_event_collector
