/**
 * @file trigger_time_collection_strategy.h
 * @brief Traditional trigger-time event collection strategy.
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>

#include "nalu_event_collector/collector/strategies/collection_strategy.h"
#include "nalu_event_collector/collector/strategies/trigger_time/trigger_time_collection_strategy_config.h"
#include "nalu_event_collector/config/event_builder_config.h"
#include "nalu_event_collector/timing/time_difference_calculator.h"

namespace nalu_event_collector {

/**
 * @brief Forms one output event for each inferred hardware trigger.
 *
 * Packets are first assembled into indivisible per-channel groups. Complete
 * groups are then matched by trigger timestamp. External-trigger events require
 * all configured channel groups; self-trigger events require only the channel
 * groups actually observed for that trigger.
 */
class TriggerTimeCollectionStrategy final : public CollectionStrategy {
   public:
    TriggerTimeCollectionStrategy(
        const EventBuilderConfig& config,
        const TriggerTimeCollectionStrategyConfig& strategy_config = {});

    void collect(const std::vector<Packet>& packets) override;
    void poll() override;
    void flush() override;
    EventBuffer& event_buffer() override { return event_buffer_; }

    /**
     * @brief Transfer the ordered prefix of ready events.
     *
     * Used by composite strategies. When @p finalize_complete_groups is true,
     * a self-trigger event containing only complete channel groups may be
     * finalized without waiting for its normal association timeout.
     */
    std::vector<std::unique_ptr<Event>> take_ready_events(
        bool finalize_complete_groups = false);

   private:
    struct ChannelGroupKey {
        uint8_t channel;
        uint32_t trigger_time;
    };

    struct PendingChannelGroup {
        ChannelGroupKey key;
        std::vector<Packet> packets;
        std::chrono::steady_clock::time_point created_at;
    };

    struct CompletedChannelGroup {
        ChannelGroupKey key;
        std::chrono::steady_clock::time_point completed_at;
    };

    enum class EventDisposition { Ready, Drop, Wait };

    bool is_configured_channel(uint8_t channel) const;
    bool is_recently_completed(const ChannelGroupKey& key) const;
    void collect_packet(const Packet& packet);
    void prune_channel_groups();
    EventDisposition classify_event(const Event& event,
                                    bool finalize_complete_groups) const;

    std::vector<int> channels_;
    int windows_;
    uint32_t event_completion_time_us_;
    uint32_t event_index_ = 0;
    std::vector<PendingChannelGroup> pending_channel_groups_;
    std::vector<CompletedChannelGroup> completed_channel_groups_;
    TimeDifferenceCalculator time_diff_calculator_;
    EventBuffer event_buffer_;
};

}  // namespace nalu_event_collector
