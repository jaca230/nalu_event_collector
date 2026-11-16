/**
 * @file trigger_time_collection_strategy.cpp
 * @brief Implements per-trigger packet collection.
 */

#include "nalu_event_collector/collector/strategies/trigger_time/trigger_time_collection_strategy.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <stdexcept>

namespace nalu_event_collector {

TriggerTimeCollectionStrategy::TriggerTimeCollectionStrategy(
    const EventBuilderConfig& config,
    const TriggerTimeCollectionStrategyConfig& strategy_config)
    : channels_(strategy_config.channels),
      windows_(strategy_config.windows),
      event_completion_time_us_(strategy_config.event_completion_time_us),
      time_diff_calculator_(strategy_config.max_trigger_time,
                            strategy_config.time_threshold),
      event_buffer_(config.max_events_in_buffer, time_diff_calculator_,
                    strategy_config.max_lookback, channels_,
                    static_cast<uint8_t>(windows_), config.event_header,
                    config.event_trailer, strategy_config.trigger_type,
                    strategy_config.wlc_mode, strategy_config.time_threshold,
                    strategy_config.clock_frequency,
                    strategy_config.event_completion_time_us) {
    if (windows_ <= 0 || windows_ > 255) {
        throw std::invalid_argument("windows must be between 1 and 255.");
    }
}

void TriggerTimeCollectionStrategy::collect(
    const std::vector<Packet>& packets) {
    prune_channel_groups();
    for (const auto& packet : packets) {
        collect_packet(packet);
    }
}

void TriggerTimeCollectionStrategy::poll() { prune_channel_groups(); }

void TriggerTimeCollectionStrategy::flush() { prune_channel_groups(); }

bool TriggerTimeCollectionStrategy::is_configured_channel(
    uint8_t channel) const {
    return std::find(channels_.begin(), channels_.end(),
                     static_cast<int>(channel)) != channels_.end();
}

bool TriggerTimeCollectionStrategy::is_recently_completed(
    const ChannelGroupKey& key) const {
    return std::any_of(
        completed_channel_groups_.begin(), completed_channel_groups_.end(),
        [&key](const CompletedChannelGroup& completed) {
            return completed.key.channel == key.channel &&
                   completed.key.trigger_time == key.trigger_time;
        });
}

void TriggerTimeCollectionStrategy::collect_packet(const Packet& packet) {
    if (!is_configured_channel(packet.channel)) {
        spdlog::warn("Dropping packet from unconfigured channel {}",
                     packet.channel);
        return;
    }

    const ChannelGroupKey key{packet.channel, packet.trigger_time};
    if (is_recently_completed(key)) {
        spdlog::warn(
            "Dropping duplicate packet for completed channel group: "
            "channel={}, "
            "trigger_time={}",
            packet.channel, packet.trigger_time);
        return;
    }

    auto pending = std::find_if(
        pending_channel_groups_.begin(), pending_channel_groups_.end(),
        [&key](const PendingChannelGroup& group) {
            return group.key.channel == key.channel &&
                   group.key.trigger_time == key.trigger_time;
        });

    if (pending == pending_channel_groups_.end()) {
        pending_channel_groups_.push_back(
            PendingChannelGroup{key, {}, std::chrono::steady_clock::now()});
        pending = std::prev(pending_channel_groups_.end());
        pending->packets.reserve(static_cast<size_t>(windows_));
    }

    pending->packets.push_back(packet);
    if (pending->packets.size() < static_cast<size_t>(windows_)) {
        return;
    }

    event_buffer_.add_packet_group(pending->packets, event_index_);
    completed_channel_groups_.push_back(
        CompletedChannelGroup{pending->key, std::chrono::steady_clock::now()});
    pending_channel_groups_.erase(pending);
}

void TriggerTimeCollectionStrategy::prune_channel_groups() {
    const auto now = std::chrono::steady_clock::now();
    const auto timeout = std::chrono::microseconds(event_completion_time_us_);

    pending_channel_groups_.erase(
        std::remove_if(pending_channel_groups_.begin(),
                       pending_channel_groups_.end(),
                       [now, timeout](const PendingChannelGroup& group) {
                           if (now - group.created_at < timeout) {
                               return false;
                           }
                           spdlog::warn(
                               "Dropping incomplete channel group: channel={}, "
                               "trigger_time={}, packets={}",
                               group.key.channel, group.key.trigger_time,
                               group.packets.size());
                           return true;
                       }),
        pending_channel_groups_.end());

    completed_channel_groups_.erase(
        std::remove_if(completed_channel_groups_.begin(),
                       completed_channel_groups_.end(),
                       [now, timeout](const CompletedChannelGroup& group) {
                           return now - group.completed_at >= timeout;
                       }),
        completed_channel_groups_.end());
}

TriggerTimeCollectionStrategy::EventDisposition
TriggerTimeCollectionStrategy::classify_event(
    const Event& event, bool finalize_complete_groups) const {
    if (event.is_event_complete() ||
        (finalize_complete_groups && event.has_complete_channel_groups())) {
        return EventDisposition::Ready;
    }
    if (!event.has_timed_out()) {
        return EventDisposition::Wait;
    }
    if (event.uses_time_based_completion() &&
        (event.get_trigger_type() != Event::TriggerType::Internal ||
         event.has_complete_channel_groups())) {
        return EventDisposition::Ready;
    }
    return EventDisposition::Drop;
}

std::vector<std::unique_ptr<Event>>
TriggerTimeCollectionStrategy::take_ready_events(
    bool finalize_complete_groups) {
    auto events = event_buffer_.get_events_after_index_inclusive(0);
    std::vector<EventDisposition> dispositions;
    dispositions.reserve(events.size());

    size_t processed_count = 0;
    for (const auto* event : events) {
        const auto disposition =
            classify_event(*event, finalize_complete_groups);
        if (disposition == EventDisposition::Wait) {
            break;
        }
        dispositions.push_back(disposition);
        ++processed_count;
    }

    auto processed =
        event_buffer_.take_events_before_index_exclusive(processed_count);
    std::vector<std::unique_ptr<Event>> ready;
    ready.reserve(processed.size());
    for (size_t i = 0; i < processed.size(); ++i) {
        if (dispositions[i] == EventDisposition::Ready) {
            ready.push_back(std::move(processed[i]));
        } else {
            spdlog::warn(
                "Dropping incomplete trigger event index {} before time "
                "batching",
                processed[i]->header.index);
        }
    }
    return ready;
}

}  // namespace nalu_event_collector
