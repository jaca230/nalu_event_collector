#ifndef NALU_EVENT_COLLECTOR_H
#define NALU_EVENT_COLLECTOR_H

#include <string>
#include <vector>
#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>  // For thread management
#include <mutex>   // For thread safety (mutex)
#include "nalu_event_builder.h"
#include "nalu_packet_parser.h"
#include "nalu_udp_receiver.h"
#include "nalu_event_collector_params.h"  // Include the parameters header
#include "nalu_collector_timing_data.h"  // Include the timing data struct

/**
 * @class NaluEventCollector
 * @brief Collects, processes, and manages events from UDP data streams.
 * 
 * This class is responsible for managing the collection process of NALU (Network Abstraction Layer Unit)
 * events from incoming UDP streams. It utilizes a receiver to fetch raw packet data, parses the data into 
 * packets, and builds events from these packets. The collector operates in a separate thread, continuously 
 * collecting and processing data. It also computes performance statistics such as data rate and processing times.
 */
class NaluEventCollector {
public:
    /**
     * @brief Constructor for NaluEventCollector.
     * @param params The parameters to initialize the collector with.
     *        These include UDP receiver, packet parser, event builder, and sleep time between cycles.
     */
    NaluEventCollector(const NaluEventCollectorParams& params);

    /**
     * @brief Destructor for NaluEventCollector.
     *        Stops the collector thread and cleans up resources.
     */
    ~NaluEventCollector();

    /**
     * @brief Starts the event collector.
     * 
     * This method starts the UDP receiver and launches a dedicated thread for collecting data.
     */
    void start();

    /**
     * @brief Stops the event collector.
     * 
     * This method stops the UDP receiver and terminates the collection thread.
     */
    void stop();

    /**
     * @brief Collects data from the receiver, parses it, and builds events.
     * 
     * This method performs the data collection, parsing, and event building in a single cycle.
     * It computes the time taken for each step and updates the performance statistics.
     */
    void collect();

    /**
     * @brief Prints the rolling average performance statistics.
     * 
     * This method prints the rolling averages for data rate, parse time, event time, 
     * UDP time, and data processed.
     */
    void printPerformanceStats();

    /**
     * @brief Retrieves the events that have been collected.
     * 
     * This method returns a list of events that are considered complete, based on the 
     * configured windows and channels.
     * 
     * @return A vector of pointers to complete events.
     */
    std::vector<NaluEvent*> get_events();

    /**
     * @brief Retrieves the current timing data for the collector.
     * 
     * This method provides the timing data that tracks the performance metrics for the 
     * most recent collection cycle.
     * 
     * @return The current timing data.
     */
    NaluCollectorTimingData get_timing_data();

    /**
     * @brief Retrieves both timing data and events.
     * 
     * This method provides a pair consisting of the current timing data and the events 
     * collected in the most recent cycle.
     * 
     * @return A pair containing the timing data and the events.
     */
    std::pair<NaluCollectorTimingData, std::vector<NaluEvent*>> get_data();

    /**
     * @brief Clears the collected events that have already been processed.
     * 
     * This method clears events from the buffer that have already been processed up to the 
     * last event index.
     */
    void clear_events();

    /**
     * @brief Accessor for the UDP receiver.
     * 
     * @return The UDP receiver used by the event collector.
     */
    NaluUdpReceiver& get_receiver() { return receiver; }

    /**
     * @brief Accessor for the packet parser.
     * 
     * @return The packet parser used by the event collector.
     */
    NaluPacketParser& get_parser() { return parser; }

    /**
     * @brief Accessor for the event builder.
     * 
     * @return The event builder used by the event collector.
     */
    NaluEventBuilder& get_event_builder() { return event_builder; }

private:
    /**
     * @brief Main collection loop.
     * 
     * This method runs in a separate thread, continuously collecting and processing data.
     */
    void collectionLoop();

    NaluUdpReceiver receiver;               /**< The UDP receiver that fetches the data. */
    NaluPacketParser parser;                 /**< The parser used to process packets. */
    NaluEventBuilder event_builder;          /**< The event builder used to build events from packets. */
    std::atomic<bool> running;               /**< Indicates whether the collector is running. */
    size_t last_event_index;                 /**< The index of the last processed event. */
    size_t cycle_count;                      /**< The number of cycles the collector has completed. */
    
    // Rolling averages
    double avg_data_rate = 0;                /**< The rolling average of the data rate. */
    double avg_parse_time = 0;               /**< The rolling average of the parse time. */
    double avg_event_time = 0;               /**< The rolling average of the event building time. */
    double avg_total_time = 0;               /**< The rolling average of the total time per cycle. */
    double avg_data_processed = 0;           /**< The rolling average of the data processed per cycle. */
    double avg_udp_time = 0;                 /**< The rolling average of the UDP processing time. */

    std::thread collector_thread_;           /**< The thread that runs the collection loop. */
    std::mutex data_mutex_;                  /**< Mutex to ensure thread safety when accessing shared data. */

    NaluCollectorTimingData timing_data;     /**< The timing data for the current cycle. */
    NaluCollectorTimingData timing_data_for_more_recent_get_events_call; /**< Timing data for the most recent get_events call. */

    std::chrono::microseconds sleep_time_us; /**< Sleep time between cycles in microseconds. */
};

#endif // NALU_EVENT_COLLECTOR_H
