/**
 * @file collector.cpp
 * @brief Implements the top-level collection pipeline orchestration.
 */

#include "nalu_event_collector/collector/collector.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <spdlog/spdlog.h>

namespace nalu_event_collector {

namespace {

constexpr int kTableColumnWidth = 23;

void print_table_separator(std::ostream& stream, size_t columns) {
    for (size_t i = 0; i < columns; ++i) {
        stream << '+'
               << std::string(static_cast<size_t>(kTableColumnWidth + 2), '-');
    }
    stream << "+\n";
}

void print_table_row(std::ostream& stream, const std::vector<std::string>& columns) {
    stream << std::setfill(' ');
    for (const auto& column : columns) {
        stream << "| " << std::setw(kTableColumnWidth) << std::right << column << ' ';
    }
    stream << "|\n";
}

std::string format_fixed(double value, int precision = 3) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(precision) << value;
    return stream.str();
}

std::string format_integer(size_t value) {
    return std::to_string(value);
}

}  // namespace

Collector::Collector(const CollectorConfig& config)
    : receiver_(config.udp_receiver),
      parser_(config.packet_parser),
      event_builder_(config.event_builder),
      running_(false),
      last_event_index_(0),
      cycle_count_(0),
      sleep_time_us_(config.sleep_time_us) {
    receiver_.getDataBuffer().setOverflowCallback([]() {
        throw std::runtime_error("UdpDataBuffer overflow detected");
    });
}

Collector::~Collector() { stop(); }

void Collector::start() {
    if (running_) {
        return;
    }

    running_ = true;
    receiver_.start();
    collector_thread_ = std::thread(&Collector::collectionLoop, this);
}

void Collector::stop() {
    if (!running_) {
        return;
    }

    running_ = false;
    receiver_.stop();
    if (collector_thread_.joinable()) {
        collector_thread_.join();
    }
}

void Collector::collect() {
    const auto start_time = std::chrono::steady_clock::now();
    const auto udp_start = std::chrono::steady_clock::now();
    std::vector<uint8_t> data = receiver_.getDataBuffer().getAllBytes();
    const auto udp_end = std::chrono::steady_clock::now();

    const double udp_time = std::chrono::duration<double>(udp_end - udp_start).count();
    const size_t data_size = data.size();

    if (data.empty()) {
        return;
    }

    const auto parse_start = std::chrono::steady_clock::now();
    std::vector<Packet> packets = parser_.process_stream(data);
    const auto parse_end = std::chrono::steady_clock::now();

    if (packets.empty()) {
        return;
    }

    const auto event_start = std::chrono::steady_clock::now();
    event_builder_.collect_events(packets);
    const auto event_end = std::chrono::steady_clock::now();
    const auto total_end = std::chrono::steady_clock::now();

    const double total_time = std::chrono::duration<double>(total_end - start_time).count();
    const double parse_time = std::chrono::duration<double>(parse_end - parse_start).count();
    const double event_time = std::chrono::duration<double>(event_end - event_start).count();
    const double data_rate = (data_size / (1024.0 * 1024.0)) / total_time;

    std::lock_guard<std::mutex> lock(data_mutex_);
    timing_data_.collection_cycle_index = cycle_count_;
    timing_data_.collection_cycle_timestamp_ns =
        std::chrono::duration_cast<std::chrono::nanoseconds>(start_time.time_since_epoch()).count();
    timing_data_.udp_time = udp_time;
    timing_data_.parse_time = parse_time;
    timing_data_.event_time = event_time;
    timing_data_.total_time = total_time;
    timing_data_.data_processed = data_size;
    timing_data_.data_rate = data_rate;

    ++cycle_count_;
    avg_data_rate_ += (data_rate - avg_data_rate_) / cycle_count_;
    avg_parse_time_ += (parse_time - avg_parse_time_) / cycle_count_;
    avg_event_time_ += (event_time - avg_event_time_) / cycle_count_;
    avg_total_time_ += (total_time - avg_total_time_) / cycle_count_;
    avg_data_processed_ += (data_size - avg_data_processed_) / cycle_count_;
    avg_udp_time_ += (udp_time - avg_udp_time_) / cycle_count_;
}

void Collector::collectionLoop() {
    while (running_) {
        collect();
        if (sleep_time_us_.count() > 0) {
            std::this_thread::sleep_for(sleep_time_us_);
        }
    }
}

void Collector::log_skipped_incomplete_events(const std::vector<Event*>& new_events,
                                              size_t complete_event_count) const {
    size_t skipped = 0;
    const size_t prefix_len = std::min(new_events.size(), complete_event_count);
    for (size_t i = 0; i < prefix_len; ++i) {
        const auto* event = new_events[i];
        if (event->is_event_complete()) {
            continue;
        }

        ++skipped;
        const int active_channels = __builtin_popcountll(event->header.channel_mask);
        const int expected_packets =
            static_cast<int>(event->header.num_windows) * active_channels;
        const auto age_us = std::chrono::duration_cast<std::chrono::microseconds>(
                                std::chrono::steady_clock::now() - event->get_creation_timestamp())
                                .count();
        std::ostringstream channel_mask_stream;
        channel_mask_stream << std::hex << event->header.channel_mask;
        spdlog::warn(
            "Skipped incomplete event index {}: packets={}/{}, windows={}, active_channels={}, "
            "reference_time={}, age_us={}, channel_mask=0x{}",
            event->header.index,
            event->header.num_packets,
            expected_packets,
            event->header.num_windows,
            active_channels,
            event->header.reference_time,
            age_us,
            channel_mask_stream.str());
    }

    if (skipped > 0) {
        spdlog::warn("Advancing past {} incomplete event(s) while returning {} complete event(s)",
                     skipped,
                     complete_event_count);
    }
}

std::vector<Event*> Collector::get_events() { return get_data().second; }

CollectorTimingData Collector::get_timing_data() {
    std::lock_guard<std::mutex> lock(data_mutex_);
    return timing_data_;
}

std::pair<CollectorTimingData, std::vector<Event*>> Collector::get_data() {
    std::lock_guard<std::mutex> lock(data_mutex_);

    std::vector<Event*> new_events =
        event_builder_.get_event_buffer().get_events_after_index_inclusive(last_event_index_);

    std::vector<Event*> complete_events;
    complete_events.reserve(new_events.size());
    for (auto* event : new_events) {
        if (event->is_event_complete()) {
            complete_events.push_back(event);
        }
    }

    log_skipped_incomplete_events(new_events, complete_events.size());
    last_event_index_ += complete_events.size();
    return {timing_data_, complete_events};
}

void Collector::clear_events() {
    std::lock_guard<std::mutex> lock(data_mutex_);
    if (last_event_index_ == 0) {
        return;
    }

    const size_t events_removed =
        event_builder_.get_event_buffer().remove_events_before_index_exclusive(last_event_index_);
    last_event_index_ -= events_removed;
}

void Collector::printPerformanceStats() {
    std::lock_guard<std::mutex> lock(data_mutex_);

    std::cout << "\nRolling Average (" << cycle_count_ << ")\n";
    print_table_separator(std::cout, 6);
    print_table_row(std::cout,
                    {"Avg Data Rate (MB/s)",
                     "Avg Parse Time (us)",
                     "Avg Event Time (us)",
                     "Avg UDP Time (us)",
                     "Avg Total Time (us)",
                     "Avg Data (bytes)"});
    print_table_row(std::cout,
                    {format_fixed(avg_data_rate_),
                     format_fixed(avg_parse_time_ * 1e6),
                     format_fixed(avg_event_time_ * 1e6),
                     format_fixed(avg_udp_time_ * 1e6),
                     format_fixed(avg_total_time_ * 1e6),
                     format_integer(static_cast<size_t>(avg_data_processed_))});
    print_table_separator(std::cout, 6);
}

}  // namespace nalu_event_collector
