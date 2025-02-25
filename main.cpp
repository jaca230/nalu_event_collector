#include "nalu_udp_receiver.h"
#include "nalu_packet_parser.h"
#include "nalu_event_builder.h"
#include <iostream>
#include <csignal>
#include <chrono>
#include <thread>
#include <iomanip>  // For std::setw and std::setfill

std::atomic<bool> running(true);

void signalHandler(int signum) {
    std::cout << "\nShutting down, please wait...\n";
    running = false;
}

int main() {
    std::signal(SIGINT, signalHandler);
    std::vector<int> channels = {0, 1};
    int windows = 31;
    int time_threshold = 5000;

    NaluPacketParser parser;
    NaluEventBuilder event_builder(channels, windows, time_threshold);
    NaluEventBuffer& event_buffer = event_builder.get_event_buffer();
    event_buffer.set_max_events(1000);
    
    // UDP receiver setup
    NaluUdpReceiver receiver("192.168.1.1", 12345);
    NaluUdpDataBuffer& udp_buffer = receiver.getDataBuffer();  // Use a reference here

    receiver.getDataBuffer().setOverflowCallback([]() {
        std::cerr << "Buffer overflow occurred!\n";
    });

    receiver.start();

    std::chrono::steady_clock::time_point last_timestamp = std::chrono::steady_clock::now();

    while (running) {
        auto start_time = std::chrono::steady_clock::now();

        std::vector<uint8_t> data = udp_buffer.getAllBytes();
        size_t data_size = data.size();

        if (!data.empty()) {
            auto parse_start = std::chrono::steady_clock::now();
            std::vector<NaluPacket> packets = parser.process_stream(data);
            auto parse_end = std::chrono::steady_clock::now();

            if (!packets.empty()) {
                auto event_start = std::chrono::steady_clock::now();
                event_builder.collect_events(packets);
                auto event_end = std::chrono::steady_clock::now();

                std::vector<NaluEvent*> new_events = event_buffer.get_events_after_timestamp(last_timestamp);
                std::chrono::steady_clock::time_point latest_complete_timestamp = last_timestamp;

                for (auto* event : new_events) {  // Use raw pointer
                    if (event->is_event_complete(windows, channels)) {
                        latest_complete_timestamp = event->get_creation_timestamp();
                    }
                }

                if (latest_complete_timestamp != last_timestamp) {
                    last_timestamp = latest_complete_timestamp;
                    event_buffer.remove_events_before_timestamp(last_timestamp);
                }

                auto total_end = std::chrono::steady_clock::now();

                double total_time = std::chrono::duration<double>(total_end - start_time).count();
                double parse_time = std::chrono::duration<double>(parse_end - parse_start).count();
                double event_time = std::chrono::duration<double>(event_end - event_start).count();
                double data_rate = (data_size / (1024.0 * 1024.0)) / total_time;  // MB/s

                std::cout << "Data Rate: " << std::fixed << std::setprecision(2) << data_rate << " MB/s, "
                          << "Parse Time: " << parse_time * 1000 << " ms, "
                          << "Event Time: " << event_time * 1000 << " ms, "
                          << "Total Processing Time: " << total_time * 1000 << " ms\n";
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    receiver.stop();
    return 0;
}
