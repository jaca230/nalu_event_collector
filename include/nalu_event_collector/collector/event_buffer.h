/**
 * @file event_buffer.h
 * @brief Thread-safe storage and lookup helpers for in-progress events.
 */

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <sys/types.h>
#include <vector>

#include "nalu_event_collector/data/event.h"
#include "nalu_event_collector/timing/time_difference_calculator.h"

namespace nalu_event_collector {

/**
 * @brief Owns the rolling buffer of events built from incoming packets.
 *
 * The buffer supports timestamp-based and index-based lookups, bounded storage,
 * and packet insertion logic that either appends to a matching event or opens a
 * new one.
 */
class EventBuffer {
  public:
    /** @brief Construct an event buffer with event-matching configuration. */
    EventBuffer(size_t max_events,
                TimeDifferenceCalculator& time_diff_calculator,
                size_t max_lookback,
                const std::vector<int>& channels,
                uint8_t windows,
                uint16_t event_header,
                uint16_t event_trailer,
                std::string trigger_type,
                bool wlc_mode,
                uint32_t time_threshold,
                uint32_t clock_frequency,
                uint32_t event_completion_time_us);

    /** @brief Destroy the buffer. */
    ~EventBuffer();

    /** @brief Insert a fully constructed event object into the buffer. */
    void add_event(std::unique_ptr<Event> event);

    /** @brief Return the underlying owned event container. */
    std::vector<std::unique_ptr<Event>>& get_events();

    /** @brief Return the newest event in the buffer. */
    Event& get_latest_event();

    /** @brief Return the event at a specific buffer index. */
    Event& get_event_by_index(size_t index);

    /** @brief Install a callback invoked before throwing on overflow. */
    void set_on_overflow_callback(std::function<void()> callback);

    /** @brief Change the maximum number of retained events. */
    void set_max_events(size_t max_events);

    /** @brief Return events whose creation timestamp is at or after @p timestamp. */
    std::vector<Event*> get_events_after_timestamp(
        const std::chrono::steady_clock::time_point& timestamp,
        ssize_t seed_index = -1) const;

    /** @brief Remove events older than @p timestamp and return the removal count. */
    size_t remove_events_before_timestamp(
        const std::chrono::steady_clock::time_point& timestamp,
        ssize_t seed_index = -1);

    /** @brief Return all events from @p index onward. */
    std::vector<Event*> get_events_after_index_inclusive(size_t index) const;

    /** @brief Remove all events before @p index and return the removal count. */
    size_t remove_events_before_index_exclusive(size_t index);

    /** @brief Route a packet into an existing event or create a new event. */
    void add_packet(const Packet& packet, bool& in_safety_buffer_zone, uint32_t& event_index);

    /** @brief Clear all buffered events. */
    void clear();

  private:
    void add_event_helper(std::unique_ptr<Event>& event);

    mutable std::mutex buffer_mutex_;
    std::vector<std::unique_ptr<Event>> events_;
    size_t max_events_;
    std::function<void()> overflow_callback_;
    TimeDifferenceCalculator& time_diff_calculator_;
    size_t max_lookback_;
    size_t max_event_size_;
    uint8_t windows_;
    uint64_t channel_mask_;
    uint16_t event_header_;
    uint16_t event_trailer_;
    uint8_t extra_info_;
    bool use_time_based_completion_;
    uint32_t time_threshold_;
    uint32_t clock_frequency_;
    uint32_t event_completion_time_us_;
};

}  // namespace nalu_event_collector
