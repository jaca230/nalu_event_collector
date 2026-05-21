/**
 * @file collector_timing_data.h
 * @brief Data model describing timing and throughput for one collection cycle.
 */

#pragma once

#include <cstddef>
#include <cstring>

namespace nalu_event_collector {

/**
 * @brief Timing and throughput summary for a single collection cycle.
 */
struct CollectorTimingData {
    /** @brief Monotonic collection-cycle index. */
    size_t collection_cycle_index = 0;

    /** @brief Cycle start timestamp in nanoseconds since epoch of steady clock. */
    long long collection_cycle_timestamp_ns = 0;

    /** @brief Time spent draining the UDP byte buffer, in seconds. */
    double udp_time = 0.0;

    /** @brief Time spent parsing packets, in seconds. */
    double parse_time = 0.0;

    /** @brief Time spent grouping packets into events, in seconds. */
    double event_time = 0.0;

    /** @brief End-to-end time for the cycle, in seconds. */
    double total_time = 0.0;

    /** @brief Number of payload bytes processed during the cycle. */
    size_t data_processed = 0;

    /** @brief Effective data rate during the cycle, in MiB/s. */
    double data_rate = 0.0;

    /** @brief Serialize the structure verbatim into @p buffer. */
    void serialize_to_buffer(char* buffer) const {
        if (buffer == nullptr) {
            return;
        }
        std::memcpy(buffer, this, sizeof(*this));
    }

    /** @brief Return the serialized byte size of the structure. */
    size_t get_size() const { return sizeof(*this); }
};

}  // namespace nalu_event_collector
