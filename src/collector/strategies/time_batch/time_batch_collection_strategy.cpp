/**
 * @file time_batch_collection_strategy.cpp
 * @brief Implements fixed-interval aggregation of inferred trigger events.
 */

#include "nalu_event_collector/collector/strategies/time_batch/time_batch_collection_strategy.h"

#include <limits>
#include <stdexcept>

namespace nalu_event_collector {

TimeBatchCollectionStrategy::TimeBatchCollectionStrategy(
    const EventBuilderConfig& config,
    const TimeBatchCollectionStrategyConfig& strategy_config)
    : config_(config),
      strategy_config_(strategy_config),
      trigger_strategy_(config, strategy_config.trigger_events),
      output_buffer_(config.settings.max_events_in_buffer),
      batch_duration_(strategy_config.duration_us),
      max_packets_per_batch_(
          static_cast<uint16_t>(strategy_config.max_packets)) {
    if (strategy_config.max_packets == 0 ||
        strategy_config.max_packets > std::numeric_limits<uint16_t>::max()) {
        throw std::invalid_argument(
            "collection_strategies.time_batch.max_packets must be between 1 "
            "and 65535.");
    }
}

void TimeBatchCollectionStrategy::collect(const std::vector<Packet>& packets) {
    poll();
    trigger_strategy_.collect(packets);
    drain_trigger_events(false);
    advance_deadline(std::chrono::steady_clock::now());
}

void TimeBatchCollectionStrategy::poll() {
    trigger_strategy_.poll();
    drain_trigger_events(false);
    const auto now = std::chrono::steady_clock::now();
    if (!deadline_initialized_) {
        next_deadline_ = now + batch_duration_;
        deadline_initialized_ = true;
    }
    advance_deadline(now);
}

void TimeBatchCollectionStrategy::flush() {
    trigger_strategy_.flush();
    drain_trigger_events(true);
    finalize_batch();
    deadline_initialized_ = false;
}

void TimeBatchCollectionStrategy::drain_trigger_events(
    bool finalize_complete_groups) {
    auto events = trigger_strategy_.take_ready_events(finalize_complete_groups);
    for (const auto& event : events) {
        append_trigger_event(*event);
    }
}

void TimeBatchCollectionStrategy::append_trigger_event(const Event& event) {
    if (event.header.num_packets > max_packets_per_batch_) {
        throw std::overflow_error(
            "One trigger event exceeds "
            "collection_strategies.time_batch.max_packets and "
            "cannot be batched.");
    }

    if (current_batch_ != nullptr &&
        current_batch_->header.num_packets + event.header.num_packets >
            max_packets_per_batch_) {
        finalize_batch();
    }

    const auto now = std::chrono::steady_clock::now();
    if (!deadline_initialized_) {
        next_deadline_ = now + batch_duration_;
        deadline_initialized_ = true;
    }

    if (current_batch_ == nullptr) {
        current_batch_ = std::make_unique<Event>(
            config_.settings.event_header, event.header.info, batch_index_++,
            event.header.reference_time,
            strategy_config_.trigger_events.time_threshold,
            strategy_config_.trigger_events.clock_frequency,
            strategy_config_.duration_us, event.header.packet_size, 0,
            config_.settings.event_trailer, max_packets_per_batch_,
            event.header.channel_mask,
            static_cast<uint8_t>(config_.event_source.windows), false);
    }

    for (size_t i = 0; i < event.header.num_packets; ++i) {
        current_batch_->add_packet(event.packets[i]);
    }
}

void TimeBatchCollectionStrategy::advance_deadline(
    std::chrono::steady_clock::time_point now) {
    if (!deadline_initialized_) {
        return;
    }

    if (batch_duration_.count() == 0) {
        finalize_batch();
        deadline_initialized_ = false;
        return;
    }

    while (now >= next_deadline_) {
        finalize_batch();
        next_deadline_ += batch_duration_;
    }
}

void TimeBatchCollectionStrategy::finalize_batch() {
    if (current_batch_ == nullptr || current_batch_->header.num_packets == 0) {
        current_batch_.reset();
        return;
    }

    current_batch_->mark_complete();
    output_buffer_.add_event(std::move(current_batch_));
}

}  // namespace nalu_event_collector
