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

// Main collector class
class NaluEventCollector {
public:
    NaluEventCollector(const NaluEventCollectorParams& params);
    ~NaluEventCollector();

    void start();
    void stop();
    void collect();
    void printPerformanceStats();
    std::vector<NaluEvent*> get_events();
    NaluCollectorTimingData get_timing_data();
    std::pair<NaluCollectorTimingData, std::vector<NaluEvent*>> get_data();
    void clear_events();

    NaluUdpReceiver& get_receiver() { return receiver; }
    NaluPacketParser& get_parser() { return parser; }
    NaluEventBuilder& get_event_builder() { return event_builder; }
private:
    void collectionLoop();

    NaluUdpReceiver receiver;
    NaluPacketParser parser;
    NaluEventBuilder event_builder;
    std::atomic<bool> running;
    size_t last_event_index;
    size_t cycle_count;
    
    // Rolling averages
    double avg_data_rate = 0;
    double avg_parse_time = 0;
    double avg_event_time = 0;
    double avg_total_time = 0;
    double avg_data_processed = 0;
    double avg_udp_time = 0;

    std::thread collector_thread_;  // Thread for the collector loop
    std::mutex data_mutex_;  // Mutex to protect shared data

    // Member variable to hold the timing data
    NaluCollectorTimingData timing_data;
    NaluCollectorTimingData timing_data_for_more_recent_get_events_call;

    // Sleep time between cycles (in microseconds)
    std::chrono::microseconds sleep_time_us;
};

#endif // NALU_EVENT_COLLECTOR_H
