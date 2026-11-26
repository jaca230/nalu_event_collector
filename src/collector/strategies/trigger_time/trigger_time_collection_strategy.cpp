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
    const EventBuilderConfig &config,
    const TriggerTimeCollectionStrategyConfig &strategy_config)
    : fail_on_data_loss_(config.event_source.fail_on_data_loss),
      channels_(config.event_source.channels),
      windows_(config.event_source.windows),
      event_completion_time_us_(strategy_config.event_completion_time_us),
      time_diff_calculator_(strategy_config.max_trigger_time,
                            strategy_config.time_threshold),
      event_buffer_(
          config.settings.max_events_in_buffer, time_diff_calculator_,
          strategy_config.max_lookback, channels_,
          static_cast<uint8_t>(windows_), config.settings.event_header,
          config.settings.event_trailer, config.event_source.trigger_type,
          config.event_source.wlc_mode, strategy_config.time_threshold,
          strategy_config.clock_frequency,
          strategy_config.event_completion_time_us) {
    for (int channel : channels_)
        if (channel >= 0 && channel < 64)
            channel_mask_ |= 1ULL << channel;
    if (windows_ <= 0 || windows_ > 255) {
        throw std::invalid_argument("windows must be between 1 and 255.");
    }
}

void TriggerTimeCollectionStrategy::collect(
    const std::vector<Packet> &packets) {
    finalizing_ = false;
    for (size_t offset = 0; offset < packets.size();) {
        const Packet *group = packets.data() + offset;
        const size_t count = static_cast<size_t>(windows_);
        bool contiguous = pending_channel_groups_.empty() &&
                          count <= packets.size() - offset &&
                          is_configured_channel(group[0].channel);
        if (contiguous)
            for (size_t j = 1; j < count; ++j)
                if (group[j].channel != group[0].channel ||
                    group[j].trigger_time != group[0].trigger_time) {
                    contiguous = false;
                    break;
                }
        if (contiguous) {
            if (fail_on_data_loss_)
                for (size_t j = 0; j < count; ++j) {
                    if (group[j].get_error_code())
                        throw std::runtime_error(
                            "Packet parser reported corrupted data");
                    for (size_t k = 0; k < j; ++k)
                        if (group[j].physical_position ==
                            group[k].physical_position)
                            throw std::runtime_error(
                                "Duplicate window within waveform");
                }
            // The parsed vector remains alive until this synchronous copy ends.
            // No temporary per-waveform vector or extra payload copy is needed.
            event_buffer_.add_validated_packet_group(group, count,
                                                     event_index_);
            offset += count;
        } else {
            collect_packet(group[0]);
            ++offset;
        }
    }
    prune_channel_groups();
}

void TriggerTimeCollectionStrategy::poll() { prune_channel_groups(); }

void TriggerTimeCollectionStrategy::flush() {
    finalizing_ = true;
    if (!pending_channel_groups_.empty()) {
        const auto &group = pending_channel_groups_.front();
        const auto reason =
            "Incomplete waveform at final flush: channel=" +
            std::to_string(group.key.channel) +
            " timestamp=" + std::to_string(group.key.trigger_time) +
            " packets=" + std::to_string(group.packets.size()) + "/" +
            std::to_string(windows_);
        if (fail_on_data_loss_)
            throw std::runtime_error(reason);
        spdlog::warn("{}; dropping {} partial waveform(s)", reason,
                     pending_channel_groups_.size());
        pending_channel_groups_.clear();
    }
    for (auto *event : event_buffer_.get_events_after_index_inclusive(0)) {
        // Channel groups are indivisible here. Time-based modes may contain
        // only the channels that actually fired (including external WLC).
        bool whole_groups = event->header.num_packets != 0;
        for (unsigned ch = 0; ch < 64; ++ch) {
            const auto count = event->get_channel_packet_count(ch);
            if (count && count != event->header.num_windows)
                whole_groups = false;
        }
        if (whole_groups && (event->is_event_complete() ||
                             event->uses_time_based_completion())) {
            event->mark_complete();
        } else {
            const auto reason = "Incomplete trigger at final flush: index=" +
                                std::to_string(event->header.index);
            if (fail_on_data_loss_)
                throw std::runtime_error(reason);
            spdlog::warn("{}", reason);
        }
    }
}

bool TriggerTimeCollectionStrategy::is_configured_channel(
    uint8_t channel) const {
    return channel < 64 && (channel_mask_ & (1ULL << channel));
}

void TriggerTimeCollectionStrategy::collect_packet(const Packet &packet) {
    if (fail_on_data_loss_ && packet.get_error_code())
        throw std::runtime_error("Packet parser reported corrupted data");
    if (!is_configured_channel(packet.channel)) {
        if (fail_on_data_loss_)
            throw std::runtime_error("Packet from unconfigured channel");
        spdlog::warn("Dropping packet from unconfigured channel {}",
                     packet.channel);
        return;
    }

    const ChannelGroupKey key{packet.channel, packet.trigger_time};
    auto pending = std::find_if(
        pending_channel_groups_.begin(), pending_channel_groups_.end(),
        [&key](const PendingChannelGroup &group) {
            return group.key.channel == key.channel &&
                   group.key.trigger_time == key.trigger_time;
        });

    if (pending == pending_channel_groups_.end()) {
        pending_channel_groups_.push_back(
            PendingChannelGroup{key, {}, std::chrono::steady_clock::now()});
        pending = std::prev(pending_channel_groups_.end());
        if (!reusable_packet_buffers_.empty()) {
            pending->packets = std::move(reusable_packet_buffers_.back());
            reusable_packet_buffers_.pop_back();
        } else
            pending->packets.reserve(static_cast<size_t>(windows_));
    }

    if (fail_on_data_loss_ &&
        std::any_of(pending->packets.begin(), pending->packets.end(),
                    [&](const Packet &p) {
                        return p.physical_position == packet.physical_position;
                    }))
        throw std::runtime_error("Duplicate window within waveform");
    pending->packets.push_back(packet);
    if (pending->packets.size() < static_cast<size_t>(windows_)) {
        return;
    }

    event_buffer_.add_validated_packet_group(
        pending->packets.data(), pending->packets.size(), event_index_);
    if (reusable_packet_buffers_.size() < 64) {
        pending->packets.clear();
        reusable_packet_buffers_.push_back(std::move(pending->packets));
    }
    pending_channel_groups_.erase(pending);
}

void TriggerTimeCollectionStrategy::prune_channel_groups() {
    const auto now = std::chrono::steady_clock::now();
    const auto timeout = std::chrono::microseconds(event_completion_time_us_);

    pending_channel_groups_.erase(
        std::remove_if(
            pending_channel_groups_.begin(), pending_channel_groups_.end(),
            [this, now, timeout](const PendingChannelGroup &group) {
                if (now - group.created_at < timeout) {
                    return false;
                }
                if (fail_on_data_loss_)
                    throw std::runtime_error(
                        "Incomplete waveform timed out: channel=" +
                        std::to_string(group.key.channel) +
                        " timestamp=" + std::to_string(group.key.trigger_time));
                spdlog::warn("Dropping incomplete channel group: channel={}, "
                             "trigger_time={}, packets={}",
                             group.key.channel, group.key.trigger_time,
                             group.packets.size());
                return true;
            }),
        pending_channel_groups_.end());
}

TriggerTimeCollectionStrategy::EventDisposition
TriggerTimeCollectionStrategy::classify_event(
    const Event &event, bool finalize_complete_groups) const {
    if (event.is_event_complete() ||
        (finalize_complete_groups && event.has_complete_channel_groups())) {
        return EventDisposition::Ready;
    }
    if (!finalizing_ && !finalize_complete_groups && !event.has_timed_out()) {
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
    for (const auto *event : events) {
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
            if (fail_on_data_loss_)
                throw std::runtime_error(
                    "Incomplete trigger before time batching");
            spdlog::warn(
                "Dropping incomplete trigger event index {} before time "
                "batching",
                processed[i]->header.index);
        }
    }
    return ready;
}

} // namespace nalu_event_collector
