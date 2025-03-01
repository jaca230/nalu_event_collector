#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <getopt.h>  // For argument parsing
#include "nalu_event_collector.h"

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
bool check_packet_integrity = true;  // Packet integrity check
uint16_t constructed_packet_header = 0xAAAA;  // Constructed packet header
uint16_t constructed_packet_footer = 0xFFFF;  // Constructed packet footer

double sleep_time_seconds = 0.5;  // For example, 0.5 seconds
std::chrono::microseconds sleep_time_us(static_cast<long long>(sleep_time_seconds * 1000000));

// Function to print help message
void printHelp() {
    std::cout << "Usage: nalu_event_collector [options]\n";
    std::cout << "Options:\n";
    std::cout << "  --background   Run collector in background mode\n";
    std::cout << "  --help         Show this help message\n";
}

int main(int argc, char** argv) {
    // Default values
    bool run_in_background = false;

    // Command-line argument parsing
    int option;
    while ((option = getopt(argc, argv, "")) != -1) {
        switch (option) {
            case 'b':  // --background flag
                run_in_background = true;
                break;
            case 'h':  // --help flag
                printHelp();
                return 0;
            default:
                printHelp();
                return 1;
        }
    }

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

    if (run_in_background) {
        // Start the collector in the background
        collector.start();
        
        // Run for a fixed amount of time or until user decides to stop
        std::this_thread::sleep_for(std::chrono::seconds(10));  // Run for 10 seconds (can be adjusted)
        
        // Stop the collector when done
        collector.stop();
    } else {
        // Manual collection loop
        collector.get_receiver().start();
        for (int i = 0; i < 10; ++i) {  // Run for 100 cycles (can be changed)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));  // Sleep for 10 milliseconds between cycles
    
            // Manually call collect
            collector.collect();
    
            // Get events and timing data
            std::pair<NaluCollectorTimingData, std::vector<NaluEvent*>> data = collector.get_data();
            std::vector<NaluEvent*> events = data.second;
            NaluCollectorTimingData timing_data = data.first;
           //std::vector<NaluEvent*> events = collector.get_events();
            
    
            // Print performance stats
            collector.printPerformanceStats();
    
            // Print summary about events received
            std::cout << "Summary of Events Received:\n";
            std::cout << "Total events received: " << events.size() << "\n";
            std::cout << "-------------------------------------------\n";
    

            /*
            // Serialize and print the first event's buffer (if any events exist)

            // Create a buffer to hold the serialized data
            size_t timing_buffer_size = timing_data.get_size();
            char* timing_buffer = new char[timing_buffer_size];

            // Serialize the timing data into the buffer
            timing_data.serialize_to_buffer(timing_buffer);

            // Print out the serialized buffer as a hex dump
            std::cout << "\n\033[1;32mSerialized Timing Data (Hex Dump):\033[0m\n";
            for (size_t i = 0; i < timing_buffer_size; ++i) {
                std::cout << std::hex << std::uppercase << (0xFF & static_cast<unsigned int>(timing_buffer[i])) << " ";
                if ((i + 1) % 16 == 0) {
                    std::cout << std::endl;  // Print new line after every 16 bytes for readability
                }
            }
            std::cout << std::dec << std::endl;  // Reset to decimal after printing hex values

            // Clean up the buffer
            delete[] timing_buffer;
            */


            /*
            if (!events.empty()) {
                // Get the first event
                NaluEvent* event = events[0];
                
                // Print out the event details before the serialized buffer
                std::cout << "Event Information:\n";
                std::cout << "Header: 0x" << std::hex << event->header << std::dec << "\n";
                std::cout << "Index: " << event->index << "\n";
                std::cout << "Reference Time: " << event->reference_time << "\n";
                std::cout << "Packet Size: " << static_cast<int>(event->packet_size) << "\n";
                std::cout << "Number of Packets: " << event->num_packets << "\n";
                std::cout << "Footer: 0x" << std::hex << event->footer << std::dec << "\n";
                std::cout << "Creation Timestamp: " << event->get_creation_timestamp().time_since_epoch().count() << " ns\n";
                std::cout << "-------------------------------------------\n";

                // Calculate the size of the serialized event
                size_t buffer_size = event->get_size();
                char* buffer = new char[buffer_size];

                // Serialize the event into the buffer
                event->serialize_to_buffer(buffer);

                // Print the serialized buffer in hex format
                std::cout << "Serialized Event Buffer (Hex):\n";
                for (size_t i = 0; i < buffer_size; ++i) {
                    printf("%02X ", static_cast<unsigned char>(buffer[i]));
                    if ((i + 1) % 16 == 0) std::cout << std::endl;
                }
                std::cout << std::endl;

                // Clean up the buffer
                delete[] buffer;
            }
            */

    
            // Clear events for the next cycle
            collector.clear_events();
        }
    }
    

    return 0;
}
