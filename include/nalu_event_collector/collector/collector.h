/**
 * @file collector.h
 * @brief Public interface for the top-level event collection pipeline.
 */

#pragma once

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

#include "nalu_event_collector/collector/event_builder.h"
#include "nalu_event_collector/config/collector_config.h"
#include "nalu_event_collector/data/collector_timing_data.h"
#include "nalu_event_collector/network/udp_receiver.h"
#include "nalu_event_collector/parsing/packet_parser.h"

namespace nalu_event_collector {

/**
 * @brief Orchestrates UDP reception, packet parsing, and event building.
 *
 * A `Collector` owns the full online collection pipeline. It can be driven
 * either manually with repeated calls to collect() or continuously through an
 * internal worker thread started with start().
 */
class Collector {
  public:
    /** @brief Construct a collector from a complete configuration object. */
    explicit Collector(const CollectorConfig& config);

    /** @brief Stop any running background work and release owned resources. */
    ~Collector();

    /** @brief Start the receiver and background collection thread. */
    void start();

    /** @brief Stop the receiver and join the background collection thread. */
    void stop();

    /** @brief Execute one collection cycle synchronously. */
    void collect();

    /** @brief Print rolling-average timing and throughput statistics. */
    void printPerformanceStats();

    /** @brief Return all newly available complete events. */
    std::vector<Event*> get_events();

    /** @brief Return the most recent timing data without advancing event state. */
    CollectorTimingData get_timing_data();

    /** @brief Return timing data and newly available complete events together. */
    std::pair<CollectorTimingData, std::vector<Event*>> get_data();

    /** @brief Remove events that have already been returned to the caller. */
    void clear_events();

    /** @brief Access the owned UDP receiver. */
    UdpReceiver& get_receiver() { return receiver_; }

    /** @brief Access the owned packet parser. */
    PacketParser& get_parser() { return parser_; }

    /** @brief Access the owned event builder. */
    EventBuilder& get_event_builder() { return event_builder_; }

  private:
    void collectionLoop();
    void log_skipped_incomplete_events(const std::vector<Event*>& new_events,
                                       size_t complete_event_count) const;

    UdpReceiver receiver_;
    PacketParser parser_;
    EventBuilder event_builder_;
    std::atomic<bool> running_;
    size_t last_event_index_;
    size_t cycle_count_;
    double avg_data_rate_ = 0.0;
    double avg_parse_time_ = 0.0;
    double avg_event_time_ = 0.0;
    double avg_total_time_ = 0.0;
    double avg_data_processed_ = 0.0;
    double avg_udp_time_ = 0.0;
    std::thread collector_thread_;
    std::mutex data_mutex_;
    CollectorTimingData timing_data_;
    std::chrono::microseconds sleep_time_us_;
};

}  // namespace nalu_event_collector
