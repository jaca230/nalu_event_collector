/**
 * @file event_buffer.cpp
 * @brief Implements rolling event-buffer management and packet insertion logic.
 */

#include "nalu_event_collector/collector/event_buffer.h"

#include <algorithm>
#include <stdexcept>

#include <spdlog/spdlog.h>

namespace nalu_event_collector {

EventBuffer::EventBuffer(size_t max_events,
                         TimeDifferenceCalculator& time_diff_calculator,
                         size_t max_lookback,
                         const std::vector<int>& channels,
                         uint8_t windows,
                         uint16_t event_header,
                         uint16_t event_trailer,
                         std::string trigger_type,
                         uint32_t time_threshold,
                         uint32_t clock_frequency,
                         uint32_t event_completion_time_us)
    : max_events_(max_events),
      time_diff_calculator_(time_diff_calculator),
      max_lookback_(max_lookback),
      max_event_size_(windows * channels.size() + 5),
      windows_(windows),
      channel_mask_(0),
      event_header_(event_header),
      event_trailer_(event_trailer),
      extra_info_(0),
      time_threshold_(time_threshold),
      clock_frequency_(clock_frequency),
      event_completion_time_us_(event_completion_time_us) {
    for (int channel : channels) {
        if (channel >= 0 && channel < 64) {
            channel_mask_ |= (1ULL << channel);
        }
    }

    uint8_t trigger_bits = 0;
    if (trigger_type == "ext") {
        trigger_bits = 0x1;
    } else if (trigger_type == "self") {
        trigger_bits = 0x2;
    } else if (trigger_type == "imm") {
        trigger_bits = 0x3;
    }

    extra_info_ = static_cast<uint8_t>(trigger_bits << 4);
}

EventBuffer::~EventBuffer() = default;

void EventBuffer::add_event(std::unique_ptr<Event> event) {
    std::lock_guard<std::mutex> lock(buffer_mutex_);
    add_event_helper(event);
}

std::vector<std::unique_ptr<Event>>& EventBuffer::get_events() {
    return events_;
}

Event& EventBuffer::get_latest_event() {
    std::lock_guard<std::mutex> lock(buffer_mutex_);
    if (events_.empty()) {
        throw std::out_of_range("No events in the buffer.");
    }
    return *events_.back();
}

Event& EventBuffer::get_event_by_index(size_t index) {
    std::lock_guard<std::mutex> lock(buffer_mutex_);
    if (index >= events_.size()) {
        throw std::out_of_range("Index is out of range.");
    }
    return *events_[index];
}

void EventBuffer::set_on_overflow_callback(std::function<void()> callback) {
    std::lock_guard<std::mutex> lock(buffer_mutex_);
    overflow_callback_ = std::move(callback);
}

void EventBuffer::set_max_events(size_t max_events) {
    std::lock_guard<std::mutex> lock(buffer_mutex_);
    max_events_ = max_events;
    if (events_.size() > max_events_) {
        events_.erase(events_.begin(), events_.begin() + (events_.size() - max_events_));
    }
}

size_t EventBuffer::remove_events_before_timestamp(
    const std::chrono::steady_clock::time_point& timestamp,
    ssize_t seed_index) {
    std::lock_guard<std::mutex> lock(buffer_mutex_);

    if (seed_index < 0 || static_cast<size_t>(seed_index) >= events_.size()) {
        seed_index = 0;
    }

    const auto it = std::lower_bound(
        events_.begin() + seed_index,
        events_.end(),
        timestamp,
        [](const std::unique_ptr<Event>& event, const std::chrono::steady_clock::time_point& ts) {
            return event->get_creation_timestamp() < ts;
        });

    const size_t num_removed = std::distance(events_.begin(), it);
    events_.erase(events_.begin(), it);
    return num_removed;
}

size_t EventBuffer::remove_events_before_index_exclusive(size_t index) {
    std::lock_guard<std::mutex> lock(buffer_mutex_);
    if (index > events_.size()) {
        throw std::out_of_range("Index is out of range.");
    }

    const size_t num_removed = index;
    events_.erase(events_.begin(), events_.begin() + num_removed);
    return num_removed;
}

std::vector<Event*> EventBuffer::get_events_after_timestamp(
    const std::chrono::steady_clock::time_point& timestamp,
    ssize_t seed_index) const {
    std::lock_guard<std::mutex> lock(buffer_mutex_);
    std::vector<Event*> result;

    if (seed_index < 0 || static_cast<size_t>(seed_index) >= events_.size()) {
        seed_index = 0;
    }

    auto it = std::lower_bound(
        events_.begin() + seed_index,
        events_.end(),
        timestamp,
        [](const std::unique_ptr<Event>& event, const std::chrono::steady_clock::time_point& ts) {
            return event->get_creation_timestamp() < ts;
        });

    for (; it != events_.end(); ++it) {
        result.push_back(it->get());
    }
    return result;
}

std::vector<Event*> EventBuffer::get_events_after_index_inclusive(size_t index) const {
    std::lock_guard<std::mutex> lock(buffer_mutex_);
    if (index >= events_.size()) {
        return {};
    }

    std::vector<Event*> result;
    result.reserve(events_.size() - index);
    for (size_t i = index; i < events_.size(); ++i) {
        result.push_back(events_[i].get());
    }
    return result;
}

void EventBuffer::add_packet(const Packet& packet,
                             bool& in_safety_buffer_zone,
                             uint32_t& event_index) {
    std::lock_guard<std::mutex> lock(buffer_mutex_);

    Event* matched_event = nullptr;
    const size_t events_size = events_.size();

    if (events_size > 0) {
        const size_t lookback_limit =
            in_safety_buffer_zone ? std::min(max_lookback_, events_size) : 1;

        for (size_t i = 0; i < lookback_limit; ++i) {
            if (time_diff_calculator_.is_within_threshold(
                    packet.trigger_time,
                    events_[events_size - 1 - i]->header.reference_time)) {
                matched_event = events_[events_size - 1 - i].get();
                break;
            }
        }
    }

    if (matched_event == nullptr) {
        auto new_event = std::make_unique<Event>(event_header_,
                                                 extra_info_,
                                                 event_index++,
                                                 packet.trigger_time,
                                                 time_threshold_,
                                                 clock_frequency_,
                                                 event_completion_time_us_,
                                                 packet.get_size(),
                                                 0,
                                                 event_trailer_,
                                                 static_cast<uint16_t>(max_event_size_),
                                                 channel_mask_,
                                                 windows_);
        new_event->add_packet(packet);
        add_event_helper(new_event);
        in_safety_buffer_zone = true;
        return;
    }

    matched_event->add_packet(packet);
}

void EventBuffer::clear() {
    std::lock_guard<std::mutex> lock(buffer_mutex_);
    events_.clear();
}

void EventBuffer::add_event_helper(std::unique_ptr<Event>& event) {
    if (events_.size() >= max_events_) {
        if (overflow_callback_) {
            overflow_callback_();
        }
        spdlog::error("Event buffer overflow. max_events={}", max_events_);
        throw std::overflow_error("Buffer is full. Cannot add more events.");
    }

    events_.push_back(std::move(event));
}

}  // namespace nalu_event_collector
