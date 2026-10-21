/**
 * @file event_builder.cpp
 * @brief Implements atomic channel-group assembly and event routing.
 */

#include "nalu_event_collector/collector/event_builder.h"

#include <algorithm>
#include <stdexcept>

#include <spdlog/spdlog.h>

namespace nalu_event_collector {

EventBuilder::EventBuilder(std::vector<int> channels,
                           int windows,
                           std::string trigger_type,
                           bool wlc_mode,
                           uint32_t time_threshold,
                           uint32_t max_trigger_time,
                           size_t max_lookback,
                           size_t event_max_size,
                           uint16_t event_header,
                           uint16_t event_trailer,
                           uint32_t clock_frequency,
                           uint32_t event_completion_time_us)
    : channels_(std::move(channels)),
      windows_(windows),
      trigger_type_(std::move(trigger_type)),
      time_threshold_duration_(ticks_to_duration(time_threshold, clock_frequency)),
      event_completion_time_us_(event_completion_time_us),
      time_diff_calculator_(max_trigger_time, time_threshold),
      event_buffer_(event_max_size,
                    time_diff_calculator_,
                    max_lookback,
                    channels_,
                    static_cast<uint8_t>(windows_),
                    event_header,
                    event_trailer,
                    trigger_type_,
                    wlc_mode,
                    time_threshold,
                    clock_frequency,
                    event_completion_time_us) {
    if (windows_ <= 0 || windows_ > 255) {
        throw std::invalid_argument("windows must be between 1 and 255.");
    }
}

EventBuilder::EventBuilder(const EventBuilderConfig& config)
    : EventBuilder(config.channels,
                   config.windows,
                   config.trigger_type,
                   config.wlc_mode,
                   config.time_threshold,
                   config.max_trigger_time,
                   config.max_lookback,
                   config.max_events_in_buffer,
                   config.event_header,
                   config.event_trailer,
                   config.clock_frequency,
                   config.event_completion_time_us) {}

void EventBuilder::collect_events(const std::vector<Packet>& packets) {
    prune_channel_groups();
    for (const auto& packet : packets) {
        collect_packet(packet);
    }
}

void EventBuilder::set_post_event_safety_buffer_counter_max(size_t counter_max) {
    static_cast<void>(counter_max);
}

bool EventBuilder::is_configured_channel(uint8_t channel) const {
    return std::find(channels_.begin(), channels_.end(), static_cast<int>(channel)) !=
           channels_.end();
}

bool EventBuilder::is_recently_completed(const ChannelGroupKey& key) const {
    return std::any_of(completed_channel_groups_.begin(),
                       completed_channel_groups_.end(),
                       [&key](const CompletedChannelGroup& completed) {
                           return completed.key.channel == key.channel &&
                                  completed.key.trigger_time == key.trigger_time;
                       });
}

void EventBuilder::collect_packet(const Packet& packet) {
    if (!is_configured_channel(packet.channel)) {
        spdlog::warn("Dropping packet from unconfigured channel {}", packet.channel);
        return;
    }

    const ChannelGroupKey key{packet.channel, packet.trigger_time};
    if (is_recently_completed(key)) {
        spdlog::warn("Dropping duplicate packet for completed channel group: channel={}, "
                     "trigger_time={}",
                     packet.channel,
                     packet.trigger_time);
        return;
    }

    auto pending = std::find_if(
        pending_channel_groups_.begin(),
        pending_channel_groups_.end(),
        [&key](const PendingChannelGroup& group) {
            return group.key.channel == key.channel && group.key.trigger_time == key.trigger_time;
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

void EventBuilder::prune_channel_groups() {
    const auto now = std::chrono::steady_clock::now();
    const auto timeout = std::chrono::microseconds(event_completion_time_us_);

    pending_channel_groups_.erase(
        std::remove_if(
            pending_channel_groups_.begin(),
            pending_channel_groups_.end(),
            [now, timeout](const PendingChannelGroup& group) {
                if (now - group.created_at < timeout) {
                    return false;
                }
                spdlog::warn(
                    "Dropping incomplete channel group: channel={}, trigger_time={}, packets={}",
                    group.key.channel,
                    group.key.trigger_time,
                    group.packets.size());
                return true;
            }),
        pending_channel_groups_.end());

    completed_channel_groups_.erase(
        std::remove_if(
            completed_channel_groups_.begin(),
            completed_channel_groups_.end(),
            [now, timeout](const CompletedChannelGroup& group) {
                return now - group.completed_at >= timeout;
            }),
        completed_channel_groups_.end());
}

std::chrono::steady_clock::duration EventBuilder::ticks_to_duration(uint32_t ticks,
                                                                    uint32_t clock_freq) const {
    if (clock_freq == 0) {
        return std::chrono::steady_clock::duration::zero();
    }
    const double ns = static_cast<double>(ticks) * (1e9 / static_cast<double>(clock_freq));
    return std::chrono::duration_cast<std::chrono::steady_clock::duration>(
        std::chrono::nanoseconds(static_cast<int64_t>(ns)));
}

}  // namespace nalu_event_collector
