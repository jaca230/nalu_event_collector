#ifndef NALU_COLLECTOR_TIMING_DATA_H
#define NALU_COLLECTOR_TIMING_DATA_H

#include <chrono>

// Struct to hold the timing data for each collection cycle
struct NaluCollectorTimingData {
    size_t collection_cycle_index;  // The index of the current collection cycle
    std::chrono::steady_clock::time_point collection_cycle_timestamp;  // Timestamp of the current cycle
    double udp_time;  // Time spent receiving UDP data
    double parse_time;  // Time spent parsing the packets
    double event_time;  // Time spent processing events
    double total_time;  // Total time for the cycle
    size_t data_processed;  // Amount of data processed (in bytes)
    double data_rate;  // Data rate (MB/s)
};

#endif // NALU_COLLECTOR_TIMING_DATA_H
