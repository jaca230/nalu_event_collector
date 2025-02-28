#include <iostream>
#include <chrono>
#include <thread>
#include "nalu_event_collector.h"

// Editable parameters for configuration

// UDP parameters
std::string udp_address = "192.168.1.1";  // UDP address
uint16_t udp_port = 12345;  // UDP port
size_t buffer_size = 1024 * 1024 * 100;  // Buffer size in bytes
size_t max_packet_size = 1040;  // Max packet size
int timeout_sec = 10;  // Timeout in seconds

// Event Builder Parameters
std::vector<int> channels = 
  { 0, 1, 2, 3, 4, 5, 6, 7,
    8, 9, 10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23,
    24, 25, 26, 27, 28, 29, 30, 31 };  // Channels to collect
int windows = 4;  // Number of windows
int time_threshold = 5000;  // Time threshold in milliseconds
size_t max_events_in_buffer = 1000000;  // Max events in buffer
uint32_t max_trigger_time = 16777216;  // Max trigger time
size_t max_lookback = 2;  // Max lookback
uint16_t event_header = 0xBBBB;  // Event header
uint16_t event_trailer = 0xEEEE;  // Event trailer

// Parsing Parameters
size_t packet_size = 74;  // Packet size
std::vector<uint8_t> start_marker = {0x0E};  // Start marker
std::vector<uint8_t> stop_marker = {0xFA, 0x5A};  // Stop marker
uint8_t chan_mask = 0x3F;  // Channel mask
uint8_t chan_shift = 0;  // Channel shift
uint8_t abs_wind_mask = 0x3F;  // Absolute window mask
uint8_t evt_wind_mask = 0x3F;  // Event window mask
uint8_t evt_wind_shift = 6;  // Event window shift
uint16_t timing_mask = 0xFFF;  // Timing mask
uint8_t timing_shift = 12;  // Timing shift
bool check_packet_integrity = false;  // Packet integrity check
uint16_t constructed_packet_header = 0xAAAA;  // Constructed packet header
uint16_t constructed_packet_footer = 0xFFFF;  // Constructed packet footer

double sleep_time_seconds = 0.5;  // For example, 0.5 seconds
std::chrono::microseconds sleep_time_us(static_cast<long long>(sleep_time_seconds * 1000000));

// Now you can use sleep_time_us


int main() {
    // Set up parameters for the collector
    NaluEventBuilderParams event_builder_params(channels, windows, time_threshold, max_events_in_buffer, max_trigger_time, max_lookback, event_header, event_trailer);
    NaluUdpReceiverParams udp_receiver_params(udp_address, udp_port, buffer_size, max_packet_size, timeout_sec);
    NaluPacketParserParams packet_parser_params(packet_size, start_marker, stop_marker, chan_mask, chan_shift, 
                                                abs_wind_mask, evt_wind_mask, evt_wind_shift, timing_mask, 
                                                timing_shift, check_packet_integrity, constructed_packet_header, constructed_packet_footer);

    // Create and initialize NaluEventCollectorParams with all parameters, including the new sleep_time_us
    NaluEventCollectorParams collector_params(event_builder_params, udp_receiver_params, packet_parser_params, sleep_time_us);

    // Create the collector
    NaluEventCollector collector(collector_params);

    // Start the collector
    collector.start();

    // Periodically grab and print data
    for (int i = 0; i < 10; ++i) {  // Run for 10 cycles (can be changed)
        std::this_thread::sleep_for(std::chrono::seconds(1));  // Sleep for 1 second between cycles

        // Get events and timing data
        std::vector<NaluEvent*> events = collector.get_events();
        NaluCollectorTimingData timing_data = collector.get_timing_data();

        // Print performance stats
        collector.printPerformanceStats();

        // Print summary about events received
        std::cout << "Summary of Events Received:\n";
        std::cout << "Total events received: " << events.size() << "\n";
        std::cout << "-------------------------------------------\n";

        collector.clear_events();
    }

    // Stop the collector when done
    collector.stop();

    return 0;
}
