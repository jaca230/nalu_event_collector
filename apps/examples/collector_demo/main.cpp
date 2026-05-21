#include <getopt.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <nlohmann/json.hpp>

#include "nalu_event_collector/collector/collector.h"
#include "nalu_event_collector/logging/logging.h"

using nalu_event_collector::Collector;
using nalu_event_collector::CollectorConfig;
using nalu_event_collector::CollectorTimingData;
using nalu_event_collector::Event;
using nalu_event_collector::logging::configure;

namespace {

struct AppConfig {
    CollectorConfig collector;
    std::string logging_level = "debug";
    std::string run_mode = "compact";
    bool background = false;
    int background_duration_s = 10;
    int manual_cycles = 10;
    int manual_sleep_ms = 100;
    bool print_middle_event = true;
};

void print_help() {
    std::cout << "Usage: collector_demo [options]\n"
              << "\nOptions:\n"
              << "  --config PATH   Path to config.json\n"
              << "  --mode MODE     Run mode: summary, compact, full\n"
              << "  --background    Override config and run collector in background mode\n"
              << "  --help          Show this help message\n";
}

template <typename T>
void assign_if_present(const nlohmann::json& json, const char* key, T& value) {
    if (json.contains(key)) {
        json.at(key).get_to(value);
    }
}

CollectorConfig parse_collector_config(const nlohmann::json& json) {
    CollectorConfig config;

    if (!json.contains("collector")) {
        return config;
    }

    const auto& collector = json.at("collector");
    if (collector.contains("sleep_time_us")) {
        config.sleep_time_us =
            std::chrono::microseconds(collector.at("sleep_time_us").get<long long>());
    }

    if (collector.contains("event_builder")) {
        const auto& event_builder = collector.at("event_builder");
        assign_if_present(event_builder, "channels", config.event_builder.channels);
        assign_if_present(event_builder, "trigger_type", config.event_builder.trigger_type);
        assign_if_present(event_builder, "windows", config.event_builder.windows);
        assign_if_present(event_builder, "wlc_mode", config.event_builder.wlc_mode);
        assign_if_present(event_builder, "time_threshold", config.event_builder.time_threshold);
        assign_if_present(event_builder,
                          "event_completion_time_us",
                          config.event_builder.event_completion_time_us);
        assign_if_present(event_builder,
                          "max_events_in_buffer",
                          config.event_builder.max_events_in_buffer);
        assign_if_present(event_builder,
                          "max_trigger_time",
                          config.event_builder.max_trigger_time);
        assign_if_present(event_builder, "clock_frequency", config.event_builder.clock_frequency);
        assign_if_present(event_builder, "max_lookback", config.event_builder.max_lookback);
        assign_if_present(event_builder, "event_header", config.event_builder.event_header);
        assign_if_present(event_builder, "event_trailer", config.event_builder.event_trailer);
    }

    if (collector.contains("udp_receiver")) {
        const auto& udp_receiver = collector.at("udp_receiver");
        assign_if_present(udp_receiver, "address", config.udp_receiver.address);
        assign_if_present(udp_receiver, "port", config.udp_receiver.port);
        assign_if_present(udp_receiver, "buffer_size", config.udp_receiver.buffer_size);
        assign_if_present(udp_receiver, "max_packet_size", config.udp_receiver.max_packet_size);
        assign_if_present(udp_receiver, "timeout_sec", config.udp_receiver.timeout_sec);
    }

    if (collector.contains("packet_parser")) {
        const auto& packet_parser = collector.at("packet_parser");
        assign_if_present(packet_parser, "packet_size", config.packet_parser.packet_size);
        assign_if_present(packet_parser, "start_marker", config.packet_parser.start_marker);
        assign_if_present(packet_parser, "stop_marker", config.packet_parser.stop_marker);
        assign_if_present(packet_parser, "chan_mask", config.packet_parser.chan_mask);
        assign_if_present(packet_parser, "chan_shift", config.packet_parser.chan_shift);
        assign_if_present(packet_parser, "abs_wind_mask", config.packet_parser.abs_wind_mask);
        assign_if_present(packet_parser, "evt_wind_mask", config.packet_parser.evt_wind_mask);
        assign_if_present(packet_parser, "evt_wind_shift", config.packet_parser.evt_wind_shift);
        assign_if_present(packet_parser, "timing_mask", config.packet_parser.timing_mask);
        assign_if_present(packet_parser, "timing_shift", config.packet_parser.timing_shift);
        assign_if_present(packet_parser,
                          "check_packet_integrity",
                          config.packet_parser.check_packet_integrity);
        assign_if_present(packet_parser,
                          "constructed_packet_header",
                          config.packet_parser.constructed_packet_header);
        assign_if_present(packet_parser,
                          "constructed_packet_footer",
                          config.packet_parser.constructed_packet_footer);
    }

    return config;
}

AppConfig load_config(const std::filesystem::path& config_path) {
    std::ifstream input(config_path);
    if (!input) {
        throw std::runtime_error("Failed to open config file: " + config_path.string());
    }

    const nlohmann::json json = nlohmann::json::parse(input);
    AppConfig config;
    config.collector = parse_collector_config(json);

    if (json.contains("logging")) {
        const auto& logging = json.at("logging");
        assign_if_present(logging, "level", config.logging_level);
    }

    if (json.contains("app")) {
        const auto& app = json.at("app");
        assign_if_present(app, "run_mode", config.run_mode);
        assign_if_present(app, "background", config.background);
        assign_if_present(app, "background_duration_s", config.background_duration_s);
        assign_if_present(app, "manual_cycles", config.manual_cycles);
        assign_if_present(app, "manual_sleep_ms", config.manual_sleep_ms);
        assign_if_present(app, "print_middle_event", config.print_middle_event);
    }

    return config;
}

enum class RunMode {
    Summary,
    Compact,
    Full,
};

RunMode parse_run_mode(const std::string& mode) {
    if (mode == "summary") {
        return RunMode::Summary;
    }
    if (mode == "compact") {
        return RunMode::Compact;
    }
    if (mode == "full") {
        return RunMode::Full;
    }
    throw std::invalid_argument("Invalid run mode: " + mode);
}

void print_middle_event(const std::vector<Event*>& events) {
    if (events.empty()) {
        std::cout << "No events collected.\n";
        return;
    }

    Event* middle_event = events[events.size() / 2];
    std::vector<char> buffer(middle_event->get_size());
    middle_event->serialize_to_buffer(buffer.data());

    const size_t event_size = middle_event->get_size();
    const size_t first_bytes = std::min(event_size, static_cast<size_t>(252));
    std::cout << "Serialized Middle Event (first 252 bytes):\n";
    for (size_t i = 0; i < first_bytes; i += 4) {
        std::cout << std::hex;
        for (size_t j = 0; j < 4 && i + j < first_bytes; ++j) {
            std::cout << std::setw(2) << std::setfill('0')
                      << static_cast<int>(buffer[i + j] & 0xFF) << " ";
        }
        std::cout << std::dec << '\n';
    }

    const size_t last_bytes = std::min(event_size, static_cast<size_t>(50));
    std::cout << "Serialized Middle Event (last " << last_bytes << " bytes):\n";
    const size_t start = event_size - last_bytes;
    for (size_t i = start; i < event_size; i += 4) {
        std::cout << std::hex;
        for (size_t j = 0; j < 4 && i + j < event_size; ++j) {
            std::cout << std::setw(2) << std::setfill('0')
                      << static_cast<int>(buffer[i + j] & 0xFF) << " ";
        }
        std::cout << std::dec << '\n';
    }
}

}  // namespace

int main(int argc, char** argv) {
    std::filesystem::path config_path = "config.json";
    bool force_background = false;
    std::string override_run_mode;

    while (true) {
        static option long_options[] = {{"config", required_argument, nullptr, 'c'},
                                        {"mode", required_argument, nullptr, 'm'},
                                        {"background", no_argument, nullptr, 'b'},
                                        {"help", no_argument, nullptr, 'h'},
                                        {nullptr, 0, nullptr, 0}};
        const int option = getopt_long(argc, argv, "c:m:bh", long_options, nullptr);
        if (option == -1) {
            break;
        }

        switch (option) {
            case 'c':
                config_path = optarg;
                break;
            case 'm':
                override_run_mode = optarg;
                break;
            case 'b':
                force_background = true;
                break;
            case 'h':
                print_help();
                return 0;
            default:
                print_help();
                return 1;
        }
    }

    const AppConfig app_config = load_config(config_path);
    configure(app_config.logging_level);
    const std::string selected_mode =
        override_run_mode.empty() ? app_config.run_mode : override_run_mode;
    const RunMode run_mode = parse_run_mode(selected_mode);

    Collector collector(app_config.collector);
    const bool run_in_background = force_background || app_config.background;

    if (run_in_background) {
        collector.start();
        std::this_thread::sleep_for(std::chrono::seconds(app_config.background_duration_s));
        collector.stop();
        return 0;
    }

    collector.get_receiver().start();

    for (int i = 0; i < app_config.manual_cycles; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(app_config.manual_sleep_ms));
        collector.collect();

        std::pair<CollectorTimingData, std::vector<Event*>> data = collector.get_data();
        std::vector<Event*> events = data.second;

        if (run_mode == RunMode::Compact || run_mode == RunMode::Full) {
            collector.printPerformanceStats();
        }
        std::cout << "Summary of Events Received:\n";
        std::cout << "Total events received: " << events.size() << "\n";
        std::cout << "-------------------------------------------\n";

        const bool should_print_middle_event =
            (run_mode == RunMode::Full) ||
            (app_config.print_middle_event &&
             run_mode != RunMode::Summary &&
             run_mode != RunMode::Compact);
        if (should_print_middle_event) {
            print_middle_event(events);
        }
        collector.clear_events();
    }

    collector.get_receiver().stop();
    return 0;
}
