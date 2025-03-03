#include <getopt.h>  // For argument parsing

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include "nalu_event_collector.h"
#include "nalu_event_collector_logger.h"

// Default UDP receiver parameters
std::string udp_address = "192.168.1.1";  // UDP address
uint16_t udp_port = 12345;                // UDP port
size_t buffer_size = 1024 * 1024 * 100;   // Buffer size in bytes
size_t max_packet_size = 1040;            // Max packet size
int timeout_sec = 10;                     // Timeout in seconds

// Default Event Builder parameters

std::vector<int> channels = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                             11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
                             22, 23, 24, 25, 26, 27, 28, 29, 30, 31};  // Channels to collect
                                             
int windows = 62;  // Number of windows
int time_threshold = 5000;//95373/2;  // Time threshold in clock cycles
size_t max_events_in_buffer = 1000000;  // Max events in buffer
uint32_t max_trigger_time = 16777216;   // Max trigger time
size_t max_lookback = 2;  // Max lookback
uint16_t event_header = 0xBBBB;  // Event header
uint16_t event_trailer = 0xEEEE; // Event trailer

// Default Parsing Parameters
size_t packet_size = 74;  // Packet size
std::string start_marker = "0E";    //Byte sequence indicating start of packet
std::string stop_marker = "FA5A";   //Byte sequence indicating stop of packet
uint8_t chan_mask = 0x3F;  // Channel mask
uint8_t chan_shift = 0;    // Channel shift
uint8_t abs_wind_mask = 0x3F;  // Absolute window mask
uint8_t evt_wind_mask = 0x3F;  // Event window mask
uint8_t evt_wind_shift = 6;    // Event window shift
uint16_t timing_mask = 0xFFF; // Timing mask
uint8_t timing_shift = 12;    // Timing shift
bool check_packet_integrity = true;  // Packet integrity check
uint16_t constructed_packet_header = 0xAAAA;  // Constructed packet header
uint16_t constructed_packet_footer = 0xFFFF;  // Constructed packet footer

// Sleep time in seconds (converted to microseconds)
double sleep_time_seconds = 0.5;
std::chrono::microseconds sleep_time_us(
    static_cast<long long>(sleep_time_seconds * 1000000));

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
  while ((option = getopt(argc, argv, "bh")) != -1) {
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

  // Manually set up the parameters for NaluEventBuilderParams
  NaluEventBuilderParams event_builder_params;
  event_builder_params.channels = channels;
  event_builder_params.windows = windows;
  event_builder_params.time_threshold = time_threshold;
  event_builder_params.max_events_in_buffer = max_events_in_buffer;
  event_builder_params.max_trigger_time = max_trigger_time;
  event_builder_params.max_lookback = max_lookback;
  event_builder_params.event_header = event_header;
  event_builder_params.event_trailer = event_trailer;

  // Manually set up the parameters for NaluUdpReceiverParams
  NaluUdpReceiverParams udp_receiver_params;
  udp_receiver_params.address = udp_address;
  udp_receiver_params.port = udp_port;
  udp_receiver_params.buffer_size = buffer_size;
  udp_receiver_params.max_packet_size = max_packet_size;
  udp_receiver_params.timeout_sec = timeout_sec;

  // Manually set up the parameters for NaluPacketParserParams
  NaluPacketParserParams packet_parser_params;
  packet_parser_params.packet_size = packet_size;
  packet_parser_params.start_marker = start_marker;
  packet_parser_params.stop_marker = stop_marker;
  packet_parser_params.chan_mask = chan_mask;
  packet_parser_params.chan_shift = chan_shift;
  packet_parser_params.abs_wind_mask = abs_wind_mask;
  packet_parser_params.evt_wind_mask = evt_wind_mask;
  packet_parser_params.evt_wind_shift = evt_wind_shift;
  packet_parser_params.timing_mask = timing_mask;
  packet_parser_params.timing_shift = timing_shift;
  packet_parser_params.check_packet_integrity = check_packet_integrity;
  packet_parser_params.constructed_packet_header = constructed_packet_header;
  packet_parser_params.constructed_packet_footer = constructed_packet_footer;

  // Manually create and set up NaluEventCollectorParams
  NaluEventCollectorParams collector_params;
  collector_params.event_builder_params = event_builder_params;
  collector_params.udp_receiver_params = udp_receiver_params;
  collector_params.packet_parser_params = packet_parser_params;
  collector_params.sleep_time_us = sleep_time_us;

  // Create NaluEventCollector instance with the manually set parameters
  NaluEventCollector collector(collector_params);

  NaluEventCollectorLogger::set_level(NaluEventCollectorLogger::LogLevel::DEBUG);

  if (run_in_background) {
    // Run the collector in the background
    collector.start();

    // Simulate collector running for a fixed amount of time
    std::this_thread::sleep_for(std::chrono::seconds(10));  // Run for 10 seconds

    // Stop the collector
    collector.stop();
  } else {
    // Manual collection loop
    collector.get_receiver().start();

    // Run for a number of cycles (10 cycles in this case)
    for (int i = 0; i < 10; ++i) {
      // Sleep for 10 milliseconds between cycles
      std::this_thread::sleep_for(std::chrono::milliseconds(10));

      // Call collect manually
      collector.collect();

      // Retrieve timing data and events
      std::pair<NaluCollectorTimingData, std::vector<NaluEvent*>> data = collector.get_data();
      std::vector<NaluEvent*> events = data.second;
      NaluCollectorTimingData timing_data = data.first;

      // Print performance stats
      collector.printPerformanceStats();

      // Print summary of events received
      std::cout << "Summary of Events Received:\n";
      std::cout << "Total events received: " << events.size() << "\n";
      std::cout << "-------------------------------------------\n";

      // Check if there are events in the middle
      /*
      if (!events.empty()) {
        int middle_index = events.size() / 2;  // Middle event index
        NaluEvent* middle_event = events[middle_index];

        // Serialize the middle event to a buffer
        char buffer[middle_event->get_size()];
        middle_event->serialize_to_buffer(buffer);

        // Print the serialized event as raw bytes in hexadecimal format
        std::cout << "Serialized Event (Middle Event) Buffer: ";
        for (size_t i = 0; i < middle_event->get_size(); ++i) {
            std::cout << std::hex << static_cast<int>(buffer[i] & 0xFF) << " ";  // Print each byte in hex
        }
        std::cout << std::dec << std::endl;  // Reset to decimal for subsequent output

        // Print detailed information about the middle event
        std::cout << "Middle Event Details:\n";
        std::cout << "Header: " << middle_event->header << std::endl;
        std::cout << "Info: " << static_cast<int>(middle_event->info) << std::endl;
        std::cout << "Index: " << middle_event->index << std::endl;
        std::cout << "Reference Time: " << middle_event->reference_time << std::endl;
        std::cout << "Packet Size: " << middle_event->packet_size << std::endl;
        std::cout << "Number of Packets: " << middle_event->num_packets << std::endl;
        std::cout << "Footer: " << middle_event->footer << std::endl;
        std::cout << "Timestamp: " << std::chrono::duration_cast<std::chrono::milliseconds>(middle_event->creation_timestamp.time_since_epoch()).count() << "ms\n";
      } else {
        std::cout << "No events collected.\n";
      }
      */
      



      // Clear events for the next cycle
      collector.clear_events();
    }
  }

  return 0;
}
