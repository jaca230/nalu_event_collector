#ifndef NALU_COLLECTOR_TIMING_DATA_H
#define NALU_COLLECTOR_TIMING_DATA_H

#include <chrono>
#include <cstring>  // For std::memcpy

/**
 * @struct NaluCollectorTimingData
 * @brief Holds timing data for a single collection cycle of the NaluEventCollector.
 *
 * This struct stores various time metrics and performance data related to each cycle of data collection,
 * such as time spent on receiving UDP data, parsing packets, processing events, and overall cycle performance.
 * It is used to monitor and optimize the data collection process.
 */
struct NaluCollectorTimingData {
    /**
     * @brief The index of the current collection cycle.
     * @size 8 bytes (size_t)
     */
    size_t collection_cycle_index;

    /**
     * @brief Timestamp of the current collection cycle in nanoseconds.
     * @size 8 bytes (long long)
     */
    long long collection_cycle_timestamp_ns;  // Timestamp in nanoseconds

    /**
     * @brief Time spent receiving UDP data, in seconds.
     * @size 8 bytes (double)
     */
    double udp_time;

    /**
     * @brief Time spent parsing the received packets, in seconds.
     * @size 8 bytes (double)
     */
    double parse_time;

    /**
     * @brief Time spent processing events after parsing the packets, in seconds.
     * @size 8 bytes (double)
     */
    double event_time;

    /**
     * @brief Total time for the collection cycle, in seconds.
     * @size 8 bytes (double)
     */
    double total_time;

    /**
     * @brief Amount of data processed in the current cycle, in bytes.
     * @size 8 bytes (size_t)
     */
    size_t data_processed;

    /**
     * @brief Data rate for the collection cycle, in MB/s.
     * @size 8 bytes (double)
     */
    double data_rate;

    /**
     * @brief Serializes the NaluCollectorTimingData to a buffer in one operation.
     * 
     * This method converts the struct into a byte array representation that can be transmitted or stored.
     * The buffer should be large enough to hold all data in the struct.
     * 
     * @param buffer The buffer to which the struct is serialized.
     */
    void serialize_to_buffer(char* buffer) const {
        if (!buffer) return;

        // Perform a single memcpy to serialize the entire structure.
        std::memcpy(buffer, this, sizeof(*this));
    }

    /**
     * @brief Returns the size of the NaluCollectorTimingData struct in bytes.
     * 
     * This method can be used to determine how much space is required for serialization.
     * 
     * @return The size of the struct in bytes.
     */
    size_t get_size() const {
        return sizeof(*this);  // Return the size of the struct
    }
};

#endif // NALU_COLLECTOR_TIMING_DATA_H
