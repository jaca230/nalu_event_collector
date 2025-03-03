#include "nalu_event_collector.h"

#include <iomanip>

#include "nalu_event_collector_logger.h"

NaluEventCollector::NaluEventCollector(const NaluEventCollectorParams& params)
    : receiver(params.udp_receiver_params),  // Unpack parameters for receiver
      parser(params.packet_parser_params),   // Unpack parameters for parser
      event_builder(
          params.event_builder_params),  // Unpack parameters for event_builder
      running(false),
      last_event_index(
          0),  // Initialize to -1, meaning no events have been processed yet
      cycle_count(0),
      sleep_time_us(params.sleep_time_us)  // Use the sleep time directly
{
    NaluEventCollectorLogger::debug("NaluEventCollector created.");
}

NaluEventCollector::~NaluEventCollector() {
    stop();
    NaluEventCollectorLogger::debug("NaluEventCollector destroyed.");
}

void NaluEventCollector::start() {
    if (!running) {  // Only start if not already running
        running = true;
        receiver.start();
        // Launch the collector thread
        collector_thread_ =
            std::thread(&NaluEventCollector::collectionLoop, this);
        NaluEventCollectorLogger::debug("NaluEventCollector thread started.");
    }
}

void NaluEventCollector::stop() {
    if (running) {  // Only stop if currently running
        running = false;
        receiver.stop();

        // Wait for the processLoop thread to finish
        if (collector_thread_.joinable()) {
            collector_thread_.join();
            NaluEventCollectorLogger::debug(
                "NaluEventCollector thread stopped.");
        }
    }
}

void NaluEventCollector::collect() {
    auto start_time = std::chrono::steady_clock::now();

    // Avoid debug output here as it may be performance-critical
    auto udp_start = std::chrono::steady_clock::now();
    std::vector<uint8_t> data = receiver.getDataBuffer().getAllBytes();
    auto udp_end = std::chrono::steady_clock::now();

    double udp_time =
        std::chrono::duration<double>(udp_end - udp_start).count();
    size_t data_size = data.size();
    double total_data_processed_kb = data_size / 1024.0;

    if (!data.empty()) {
        auto parse_start = std::chrono::steady_clock::now();
        std::vector<NaluPacket> packets = parser.process_stream(data);
        auto parse_end = std::chrono::steady_clock::now();

        if (!packets.empty()) {
            auto event_start = std::chrono::steady_clock::now();
            event_builder.collect_events(packets);
            auto event_end = std::chrono::steady_clock::now();

            auto total_end = std::chrono::steady_clock::now();
            double total_time =
                std::chrono::duration<double>(total_end - start_time).count();
            double parse_time =
                std::chrono::duration<double>(parse_end - parse_start).count();
            double event_time =
                std::chrono::duration<double>(event_end - event_start).count();
            double data_rate = (data_size / (1024.0 * 1024.0)) / total_time;

            // Lock mutex to update shared data
            std::lock_guard<std::mutex> lock(data_mutex_);

            // Update the NaluCollectorTimingData for this cycle
            timing_data.collection_cycle_index = cycle_count;
            timing_data.collection_cycle_timestamp_ns =
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    start_time.time_since_epoch())
                    .count();
            timing_data.udp_time = udp_time;
            timing_data.parse_time = parse_time;
            timing_data.event_time = event_time;
            timing_data.total_time = total_time;
            timing_data.data_processed = data_size;
            timing_data.data_rate = data_rate;

            // Compute rolling averages
            cycle_count++;

            avg_data_rate += (data_rate - avg_data_rate) / cycle_count;
            avg_parse_time += (parse_time - avg_parse_time) / cycle_count;
            avg_event_time += (event_time - avg_event_time) / cycle_count;
            avg_total_time += (total_time - avg_total_time) / cycle_count;
            avg_data_processed +=
                (data_size - avg_data_processed) / cycle_count;
            avg_udp_time += (udp_time - avg_udp_time) / cycle_count;
        }
    } else {
        NaluEventCollectorLogger::debug("No data received from receiver.");
    }
}

void NaluEventCollector::collectionLoop() {
    while (running) {
        collect();
        // Sleep for the specified time if it's non-negative
        if (sleep_time_us.count() > 0) {
            std::this_thread::sleep_for(sleep_time_us);
            NaluEventCollectorLogger::debug(
                "Sleeping for " + std::to_string(sleep_time_us.count()) +
                " microseconds.");
        }
    }
}

std::vector<NaluEvent*> NaluEventCollector::get_events() {
    return get_data().second;
}

NaluCollectorTimingData NaluEventCollector::get_timing_data() {
    return get_data().first;
}

std::pair<NaluCollectorTimingData, std::vector<NaluEvent*>>
NaluEventCollector::get_data() {
    // Get timing data and event data in same mutex
    std::lock_guard<std::mutex> lock(data_mutex_);

    // Fetch events after the last processed index
    std::vector<NaluEvent*> new_events =
        event_builder.get_event_buffer().get_events_after_index_inclusive(
            last_event_index);

    // Filter and return only complete events
    std::vector<NaluEvent*> complete_events;
    std::vector<int> channels = event_builder.get_channels();
    int windows = event_builder.get_windows();

    // Check each event for completion
    for (auto* event : new_events) {
        bool is_complete = event->is_event_complete(windows, channels);
        //std::cout << "Event is complete: " << is_complete << std::endl;
        //std::cout << "Event num packets: " << event->num_packets << std::endl;
        //std::cout << "Event reference time: " << event->reference_time << std::endl;
        if (is_complete) {
            complete_events.push_back(event);
        }
    }

    // Update last_event_index by counting the complete events
    last_event_index += complete_events.size();

    return {timing_data, complete_events};
}

void NaluEventCollector::clear_events() {
    std::lock_guard<std::mutex> lock(data_mutex_);
    // Clear events based on the last processed index
    if (last_event_index > 0) {
        size_t events_removed =
            event_builder.get_event_buffer()
                .remove_events_before_index_exclusive(last_event_index);
        last_event_index -= events_removed;
    }
}

void NaluEventCollector::printPerformanceStats() {
    std::lock_guard<std::mutex> lock(data_mutex_);

    std::cout << "\n\033[1;32mRolling Average (" << cycle_count
              << "):\033[0m\n";
    std::cout
        << "+-------------------------+-------------------------+----------"
           "---------------+-------------------------+---------------------"
           "----+-------------------------+\n";
    std::cout << "| " << std::setw(23) << "Avg Data Rate (MB/s)"
              << " | " << std::setw(24) << "Avg Parse Time (µs)"
              << " | " << std::setw(24) << "Avg Event Time (µs)"
              << " | " << std::setw(24) << "Avg UDP Time (µs)"
              << " | " << std::setw(24) << "Avg Total Time (µs)"
              << " | " << std::setw(23) << "Avg Data Processed (KB)"
              << " |\n";
    std::cout
        << "+-------------------------+-------------------------+----------"
           "---------------+-------------------------+---------------------"
           "----+-------------------------+\n";
    std::cout << "| " << std::setw(23) << std::fixed << std::setprecision(6)
              << avg_data_rate << " | " << std::setw(23) << avg_parse_time * 1e6
              << " | " << std::setw(23) << avg_event_time * 1e6 << " | "
              << std::setw(23) << avg_udp_time * 1e6 << " | " << std::setw(23)
              << avg_total_time * 1e6 << " | " << std::setw(23)
              << avg_data_processed / 1024.0 << " |\n";
    std::cout
        << "+-------------------------+-------------------------+----------"
           "---------------+-------------------------+---------------------"
           "----+-------------------------+\n";
}
