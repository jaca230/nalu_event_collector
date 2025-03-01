# Nalu Event Collector Library

A high-performance C++ library for collecting and processing UDP network events in real-time applications.

## Overview

The Nalu Event Collector provides methods for capturing, parsing, and processing UDP packets into structured events. It's designed for applications requiring efficient network data collection and processing with minimal overhead.

## Features

- Real-time UDP packet collection and processing
- Configurable packet parsing and event building
- Thread-safe operation with background collection
- Performance metrics and timing data
- Simple API through the NaluEventCollector class
- Custom event filtering and processing options

## Installation

### Prerequisites

- C++17 compatible compiler
- CMake (>= 3.10)
- UNIX-like operating system

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

```cpp
#include <nalu_event_collector.h>

int main() {
    // Configure the collector
    NaluEventBuilderParams builder_params;
    NaluUdpReceiverParams udp_params("192.168.1.100", 8080);
    NaluPacketParserParams parser_params;
    
    // Create collector parameters
    NaluEventCollectorParams params(
        builder_params,
        udp_params,
        parser_params,
        std::chrono::microseconds(100)  // Collection interval
    );

    // Initialize collector
    NaluEventCollector collector(params);
    
    // Start collection
    collector.start();

    // Process events
    while (running) {
        auto events = collector.get_events();
        for (const auto& event : events) {
            // Process your events
        }
        collector.clear_events();
    }

    collector.stop();
    return 0;
}
```

## API Reference

### NaluEventCollector Class

The main interface class for interacting with the library.

#### Key Methods

```cpp
// Start the event collection process
void start();

// Stop the event collection process
void stop();

// Retrieve collected events
std::vector<NaluEvent*> get_events();

// Clear the event buffer
void clear_events();

// Get performance timing data
NaluCollectorTimingData get_timing_data();

// Print performance statistics
void printPerformanceStats();
```

### Configuration Parameters

#### NaluEventCollectorParams

```cpp
struct NaluEventCollectorParams {
    NaluEventBuilderParams event_builder_params;
    NaluUdpReceiverParams udp_receiver_params;
    NaluPacketParserParams packet_parser_params;
    std::chrono::microseconds sleep_time_us;
};
```

#### NaluUdpReceiverParams

```cpp
struct NaluUdpReceiverParams {
    std::string address;  // UDP listen address
    uint16_t port;       // UDP port number
    // Additional network configuration options
};
```

## Advanced Usage

### Background Collection Mode

```cpp
NaluEventCollector collector(params);
collector.start();

// Collection happens in background thread
std::this_thread::sleep_for(std::chrono::seconds(10));

// Get accumulated events
auto events = collector.get_events();
collector.stop();
```

### Performance Monitoring

```cpp
NaluEventCollector collector(params);
collector.start();

// Get timing data
auto timing = collector.get_timing_data();
std::cout << "Average processing time: " 
          << timing.avg_processing_time_us << " us\n";

// Print detailed stats
collector.printPerformanceStats();
```

## Best Practices

1. Always call `clear_events()` after processing to prevent memory growth
2. Monitor timing data for performance optimization
3. Configure sleep time based on your application's needs
4. Use appropriate buffer sizes for your use case

## Performance Considerations

- Event processing happens in a separate thread
- Buffer sizes affect memory usage and processing latency
- Network conditions can impact collection performance
- Regular monitoring of timing data is recommended

## Error Handling

The library uses exceptions for error conditions. Main exception types:

- `NaluConfigurationError`
- `NaluNetworkError`
- `NaluProcessingError`

## Thread Safety

The NaluEventCollector class is thread-safe for:
- Concurrent calls to `get_events()`
- Simultaneous start/stop operations
- Event collection and processing

## License

MIT License. See LICENSE file for details.

## Contributing

Contributions welcome! Please read CONTRIBUTING.md for guidelines.