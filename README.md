# Nalu Event Collector Library

A high-performance C++ library for collecting and processing UDP network events in real-time applications.

## Overview

The Nalu Event Collector provides methods for capturing, parsing, and processing UDP packets into structured events. It's designed for applications requiring efficient network data collection and processing with minimal overhead.

## Documentation

[Doxygen documentation is available](https://jaca230.github.io/nalu_event_collector/html/files.html).

## Features

- Real-time UDP packet collection and processing
- Configurable packet parsing and event building
- Thread-safe operation with background collection
- Performance metrics and timing data
- Simple API through the NaluEventCollector class

## Installation

### Prerequisites

- C++17 compatible compiler
- CMake (>= 3.10)
- UNIX-like operating system
- To receive data you must have a nalu scientific board configured to send data over a 1GbE link to the host system. See my [nalu_board_controller library](https://github.com/jaca230/nalu_board_controller).

### Building and Installing

```bash
# Clone the repository
git clone https://github.com/jaca230/nalu_event_collector
cd nalu_event_collector

# Build and install
./scripts/install.sh
```

For custom installation directory:

```bash
./scripts/install.sh --prefix /custom/install/path
```

## Quick Start

Note, to receive data you must have a board configured to send data over a 1GbE link. See my [nalu_board_controller library](https://github.com/jaca230/nalu_board_controller).

```cpp
#include <getopt.h>  // For argument parsing

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include "nalu_event_collector.h"

// UDP Receiver Parameters
std::string udp_address = "192.168.1.1";  // UDP address
uint16_t udp_port = 12345;                // UDP port
size_t buffer_size = 1024 * 1024 * 100;   // Buffer size in bytes
size_t max_packet_size = 1040;            // Max packet size
int timeout_sec = 10;                     // Timeout in seconds

// Event Builder Parameters
std::vector<int> channels = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
                             11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
                             22, 23, 24, 25, 26, 27, 28, 29, 30, 31};  // Channels to collect
int windows = 4;                                          // Number of windows
int time_threshold = 5000;                                // Time threshold in clock cycles
size_t max_events_in_buffer = 1000000;                    // Max events in buffer
uint32_t max_trigger_time = 16777216;                     // Max trigger time
size_t max_lookback = 2;                                  // Max lookback
uint16_t event_header = 0xBBBB;                           // Event header
uint16_t event_trailer = 0xEEEE;                          // Event trailer

// Parsing Parameters (for Nalu HDSoc v1 board)
size_t packet_size = 74;                          // Packet size
std::vector<uint8_t> start_marker = {0x0E};       // Start marker
std::vector<uint8_t> stop_marker = {0xFA, 0x5A};  // Stop marker
uint8_t chan_mask = 0x3F;                         // Channel mask
uint8_t chan_shift = 0;                           // Channel shift
uint8_t abs_wind_mask = 0x3F;                     // Absolute window mask
uint8_t evt_wind_mask = 0x3F;                     // Event window mask
uint8_t evt_wind_shift = 6;                       // Event window shift
uint16_t timing_mask = 0xFFF;                     // Timing mask
uint8_t timing_shift = 12;                        // Timing shift
bool check_packet_integrity = true;               // Packet integrity check
uint16_t constructed_packet_header = 0xAAAA;      // Constructed packet header
uint16_t constructed_packet_footer = 0xFFFF;      // Constructed packet footer
double sleep_time_seconds = 0.5;  // For example, 0.5 seconds
std::chrono::microseconds sleep_time_us(
    static_cast<long long>(sleep_time_seconds * 1000000));


int main(int argc, char** argv) {

  // Set up parameters for the collector
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


  // Manual collection loop
  collector.get_receiver().start();
  for (int i = 0; i < 10; ++i) {  // Run for 10 cycles
      std::this_thread::sleep_for(std::chrono::milliseconds(
          10));  // Sleep for 10 milliseconds between cycles

      // Manually call collect
      collector.collect();

      // Get events and timing data
      std::pair<NaluCollectorTimingData, std::vector<NaluEvent*>> data =
          collector.get_data();
      std::vector<NaluEvent*> events = data.second;
      NaluCollectorTimingData timing_data = data.first;

      // Print performance stats
      collector.printPerformanceStats();

      // Print summary about events received
      std::cout << "Summary of Events Received:\n";
      std::cout << "Total events received: " << events.size() << "\n";
      std::cout << "-------------------------------------------\n";

      // Clear events for the next cycle
      collector.clear_events();
  }

  return 0;
}

```

## NaluEventCollector API Reference

The `NaluEventCollector` class is the core interface for interacting with the Nalu event collection library. It is designed to collect, process, and manage events from UDP streams in a multi-threaded environment. It is highly configurable and allows users to monitor performance, configure data collection intervals, and retrieve detailed timing data and events.

### Key Features

- **Event Collection**: Continuously collects events from UDP streams, processes them into packets, and builds events from these packets.
- **Performance Monitoring**: Tracks processing times (e.g., UDP processing, parsing, event building) and computes rolling averages to help monitor system performance.
- **Multi-Threaded**: Operates in a separate collection thread, which can be controlled via the `start` and `stop` methods.
- **Event Buffering**: Collects events in a buffer and allows access to events through the `get_events` method. The buffer is managed and can be cleared as needed.

---

### Key Methods

For any of the methods, you'll first need to construct the collector
```cpp
#include "nalu_event_collector.h"
//Construct "params" struct. See above.
NaluEventCollector collector(params);
```

#### `void start();`

Starts the event collection process. This method initializes the UDP receiver and spawns a background thread that continuously collects data.

- **Thread-Safe**: No concurrent access to shared data when calling this method.
- **Usage**: Call this method to begin the event collection. The process will continue until explicitly stopped.

```cpp
collector.start();
```

#### `void stop();`

Stops the event collection process, terminates the background thread, and shuts down the UDP receiver.

- **Thread-Safe**: Safe to call while the collection thread is running.
- **Usage**: Call this method when you want to stop event collection and clean up resources.

```cpp
collector.stop();
```

#### `void collect();`

Collects events from the UDP buffer and processes them based on the configured parameters. This method triggers the event collection process on demand, performing the necessary steps to gather data and handle events according to the settings specified in the `NaluEventCollectorParams`.

- **Usage**: Call this method to manually initiate event collection. It can be used in scenarios where event collection needs to be triggered based on external conditions rather than running continuously in the background.
- **Thread-Safe**: The method handles synchronization internally to ensure data integrity during the collection process, making it safe for use in multi-threaded environments.

```cpp
collector.collect();  // Manually triggers the event collection
auto [events, timing] = collector.get_data();
```

- **Note**: This method is particularly useful for more controlled event collection.

#### `std::tuple<std::vector<NaluEvent*>, NaluCollectorTimingData> get_data();`

Retrieves both the collected events and the associated timing data for the most recent collection cycle. This method combines the functionality of `get_events()` and `get_timing_data()` into a single call, returning both the events and performance metrics as a tuple.

- **Returns**: A tuple where the first element is a vector of `NaluEvent*` representing the collected events, and the second element is a `NaluCollectorTimingData` struct containing the timing metrics for the latest collection cycle.
- **Thread-Safe**: Safely accessed using internal synchronization mechanisms to ensure consistent data retrieval from multiple threads.

```cpp
auto [events, timing] = collector.get_data();
std::cout << "Number of events: " << events.size() << "\n";
std::cout << "Processing time: " << timing.total_time << " s\n";
```

- **Usage**: Use this method to obtain both the events and the performance statistics in a single call. This is especially useful when you need to process or analyze both the data and timing information together.


#### `std::vector<NaluEvent*> get_events();`

Retrieves the events collected in the most recent cycle. The events are filtered for completeness and are returned in the order they were processed.

- **Returns**: A vector of `NaluEvent*` pointers representing complete events.
- **Thread-Safe**: Locks the internal mutex to safely fetch the events while the collection thread is running.

```cpp
auto events = collector.get_events();
```

#### `void clear_events();`

Clears the event buffer by removing events that have already been processed.

- **Usage**: This can be used to reset the state and discard previously collected events, useful for preventing memory buildup.
- **Thread-Safe**: The internal mutex ensures no data races when accessing or modifying the event buffer.

```cpp
collector.clear_events();
```

#### `NaluCollectorTimingData get_timing_data();`

Returns the most recent collection cycle's timing data, including details such as processing times for UDP reception, packet parsing, and event building.

- **Returns**: `NaluCollectorTimingData` struct containing timing metrics for the latest cycle.
- **Thread-Safe**: Safely accessed using internal synchronization mechanisms.

```cpp
auto timing = collector.get_timing_data();
std::cout << "Processing time: " << timing.total_time << " s\n";
```

#### `void printPerformanceStats();`

Prints the rolling average performance statistics, including average data rate, parse time, event time, UDP time, and data processed, formatted in a human-readable table.

- **Usage**: This method is useful for monitoring the performance of the event collector over time.
- **Thread-Safe**: Internal locking is used to ensure data consistency.

```cpp
collector.printPerformanceStats();
```

---

### Configuration Parameters

#### `NaluEventCollectorParams`

A struct that holds all the configuration parameters necessary to initialize the `NaluEventCollector` class.

```cpp
struct NaluEventCollectorParams {
    NaluEventBuilderParams event_builder_params;  // Parameters for event builder
    NaluUdpReceiverParams udp_receiver_params;    // Parameters for UDP receiver
    NaluPacketParserParams packet_parser_params;  // Parameters for packet parser
    std::chrono::microseconds sleep_time_us;      // Sleep time between collection cycles
};
```

- **`event_builder_params`**: Defines how events are constructed from the parsed packets.
- **`udp_receiver_params`**: Specifies the UDP address and port to listen on, as well as other network-related parameters.
- **`packet_parser_params`**: Configures the packet parser to process incoming UDP data streams into NALU packets.
- **`sleep_time_us`**: Specifies how long the collection thread should sleep between cycles (in microseconds).

#### `NaluEventBuilderParams`

Defines the parameters used to configure the event builder, which handles constructing events from incoming NALU packets.

```cpp
struct NaluEventBuilderParams {
    std::vector<int> channels;    // List of channels for event creation
    int windows;                  // Number of windows for event creation
    int time_threshold;           // Time threshold for triggering events
    size_t max_events_in_buffer;  // Maximum number of events in the buffer
    uint32_t max_trigger_time;    // Maximum trigger time in clock cycles
    size_t max_lookback;          // Maximum lookback period for events
    uint16_t event_header;        // Header value for event
    uint16_t event_trailer;       // Trailer value for event
};
```

- **`channels`**: List of channel indices to be used for event construction (default is `{0, 1, 2, ..., 15}`).
- **`windows`**: The number of windows for event creation (default is `4`).
- **`time_threshold`**: The time threshold for event triggering, in clock cycles (default is `5000`).
- **`max_events_in_buffer`**: The maximum number of events that can be stored in the buffer (default is `1000000`).
- **`max_trigger_time`**: The maximum allowable trigger time for events, in clock cycles (default is `16777216`).
- **`max_lookback`**: The maximum lookback period for event collection (default is `2`).
- **`event_header`**: The header value to be included in the event (default is `0xBBBB`).
- **`event_trailer`**: The trailer value to be included in the event (default is `0xEEEE`).

#### `NaluUdpReceiverParams`

Defines the parameters used to configure the UDP receiver, which handles the UDP communication and receiving raw packet data.

```cpp
struct NaluUdpReceiverParams {
    std::string address;     // The UDP address to listen on
    uint16_t port;           // The UDP port to listen on
    size_t buffer_size;      // Size of the buffer for storing incoming data
    size_t max_packet_size;  // Maximum size of a single packet
    int timeout_sec;         // Timeout duration for receiving data, in seconds
};
```

- **`address`**: The UDP address to listen on (e.g., `"192.168.1.1"`).
- **`port`**: The UDP port to receive data from (e.g., `12345`).
- **`buffer_size`**: The size of the buffer used to store incoming data (default is `1024 * 1024 * 100`).
- **`max_packet_size`**: The maximum allowed size of a single packet (default is `1040`).
- **`timeout_sec`**: The timeout duration for receiving data, in seconds (default is `10`).

#### `NaluPacketParserParams`

Defines the parameters used to configure the packet parser, which processes incoming NALU packets and extracts relevant information.

```cpp
struct NaluPacketParserParams {
    size_t packet_size;         // Size of a single packet
    std::vector<uint8_t> start_marker;  // Start marker to identify the beginning of a packet
    std::vector<uint8_t> stop_marker;   // Stop marker to identify the end of a packet
    uint8_t chan_mask;          // Channel mask for packet extraction
    uint8_t chan_shift;         // Shift value for channel extraction
    uint8_t abs_wind_mask;      // Absolute window mask for packet processing
    uint8_t evt_wind_mask;      // Event window mask for packet processing
    uint8_t evt_wind_shift;     // Event window shift value
    uint16_t timing_mask;       // Mask for timing-related information in the packet
    uint8_t timing_shift;       // Shift for timing-related information
    bool check_packet_integrity;  // Flag to check packet integrity
    uint16_t constructed_packet_header;  // Header value for the constructed packet
    uint16_t constructed_packet_footer;  // Footer value for the constructed packet
};
```

- **`packet_size`**: The size of a single packet (default is `74`).
- **`start_marker`**: The start marker used to identify the beginning of a packet (default is `{0x0E}`).
- **`stop_marker`**: The stop marker used to identify the end of a packet (default is `{0xFA, 0x5A}`).
- **`chan_mask`**: The channel mask for extracting channel data from the packet (default is `0x3F`).
- **`chan_shift`**: The shift value used for channel extraction (default is `0`).
- **`abs_wind_mask`**: The absolute window mask used for packet processing (default is `0x3F`).
- **`evt_wind_mask`**: The event window mask used for packet processing (default is `0x3F`).
- **`evt_wind_shift`**: The shift value used for event window processing (default is `6`).
- **`timing_mask`**: The mask for timing-related information in the packet (default is `0xFFF`).
- **`timing_shift`**: The shift for timing-related information in the packet (default is `12`).
- **`check_packet_integrity`**: Flag to indicate whether packet integrity should be checked (default is `false`).
- **`constructed_packet_header`**: Header value for the constructed packet (default is `0xAAAA`).
- **`constructed_packet_footer`**: Footer value for the constructed packet (default is `0xFFFF`).

---

### Advanced Usage

#### Background Collection Mode

In background collection mode, the event collector runs on a separate thread, collecting data continuously. You can specify the time duration for which data is collected before stopping.

```cpp
NaluEventCollector collector(params);
collector.start();

// Collection happens in background thread
std::this_thread::sleep_for(std::chrono::seconds(10));

// Retrieve accumulated events
auto events = collector.get_events();
collector.stop();
```

- **Usage**: The collector will run in the background while you can access the collected events and monitor the performance.

#### Performance Monitoring

For performance monitoring, you can retrieve detailed timing statistics about each collection cycle, including metrics for UDP processing time, parsing time, event building time, and overall data rate.

```cpp
NaluEventCollector collector(params);
collector.start();

// Get timing data after some collection cycles
auto timing = collector.get_timing_data();
std::cout << "Average processing time: " << timing.avg_processing_time_us << " us\n";

// Print detailed performance statistics
collector.printPerformanceStats();
```

- **Use Case**: This can be used to monitor the efficiency and throughput of the event collection system over time.

---

### License

This library is licensed under the MIT License. See the `LICENSE` file for more details.

---

### Notes

- **Thread Safety**: The `NaluEventCollector` is designed to be thread-safe. The internal data structures are protected using mutexes to ensure that data is accessed and modified safely, even when multiple threads are interacting with the collector.
- **Performance Considerations**: The collector's performance can be affected by the amount of data processed. Generally, the more UDP data the collector processes at a time, the better the performance. In this way, it may be best to "chunk" collections into many events at a time.

## License

MIT License. See LICENSE file for details.

## AI in documentation
Most of this documentation was written by AI. I have reviewed it for correctness, but it is possible there are errors.