/**
 * @file event.cpp
 * @brief Implements the serialized event data model and completeness logic.
 */

#include "nalu_event_collector/data/event.h"

#include <cstring>
#include <iostream>
#include <stdexcept>

#include <spdlog/spdlog.h>

namespace nalu_event_collector {

Event::Event()
    : header{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      footer{0},
      max_packets(0),
      packets(nullptr),
      use_time_based_completion_(false),
      creation_timestamp(std::chrono::steady_clock::now()) {}

Event::Event(uint16_t hdr,
             uint8_t extra_info,
             uint32_t idx,
             uint32_t ref_time,
             uint32_t time_thresh,
             uint32_t clk_freq,
             uint32_t event_completion_time_us,
             uint16_t size,
             uint16_t num,
             uint16_t ftr,
             uint16_t max_num_packets,
             uint64_t channel_mask_value,
             uint8_t num_windows_value,
             bool use_time_based_completion)
    : header{hdr,
             extra_info,
             idx,
             ref_time,
             time_thresh,
             event_completion_time_us,
             clk_freq,
             size,
             channel_mask_value,
             num_windows_value,
             num},
      packets(std::make_unique<Packet[]>(max_num_packets)),
      footer{ftr},
      max_packets(max_num_packets),
      use_time_based_completion_(use_time_based_completion),
      creation_timestamp(std::chrono::steady_clock::now()) {}

void Event::print_event_info() const {
    std::cout << "Event Header:\n";
    std::cout << "Header: " << header.header << '\n';
    std::cout << "Info: " << static_cast<int>(header.info) << '\n';
    std::cout << "Index: " << header.index << '\n';
    std::cout << "Reference Time: " << header.reference_time << '\n';
    std::cout << "Time Threshold (ticks): " << header.time_threshold << '\n';
    std::cout << "Event Completion Time (us): " << header.event_completion_time_us << '\n';
    std::cout << "Clock Frequency (Hz): " << header.clock_frequency << '\n';
    std::cout << "Packet Size: " << header.packet_size << '\n';
    std::cout << "Channel Mask: " << header.channel_mask << '\n';
    std::cout << "Number of Windows: " << static_cast<int>(header.num_windows) << '\n';
    std::cout << "Number of Packets: " << header.num_packets << '\n';
    std::cout << "Event Footer: " << footer.footer << '\n';
}

uint8_t Event::get_error_code() const { return header.info & 0x0F; }

Event::TriggerType Event::get_trigger_type() const {
    const uint8_t trigger_bits = (header.info & 0x30) >> 4;
    switch (trigger_bits) {
        case 1:
            return TriggerType::External;
        case 2:
            return TriggerType::Internal;
        case 3:
            return TriggerType::Immediate;
        default:
            return TriggerType::Unknown;
    }
}

size_t Event::get_size() const {
    return sizeof(header) + (header.num_packets * sizeof(Packet)) + sizeof(footer);
}

void Event::serialize_to_buffer(char* buffer) const {
    if (buffer == nullptr) {
        return;
    }

    size_t offset = 0;
    std::memcpy(buffer + offset, &header, sizeof(header));
    offset += sizeof(header);
    std::memcpy(buffer + offset, packets.get(), header.num_packets * sizeof(Packet));
    offset += header.num_packets * sizeof(Packet);
    std::memcpy(buffer + offset, &footer, sizeof(footer));
}

void Event::add_packet(const Packet& packet) {
    if (header.num_packets >= max_packets) {
        spdlog::error("Attempt to add packet exceeds max packet limit. max={}, current={}",
                      max_packets,
                      header.num_packets);
        throw std::overflow_error("Maximum number of packets exceeded.");
    }

    packets[header.num_packets] = packet;
    ++header.num_packets;
}

int Event::count_active_channels(uint64_t channel_mask) const {
    int count = 0;
    while (channel_mask != 0) {
        count += static_cast<int>(channel_mask & 1U);
        channel_mask >>= 1U;
    }
    return count;
}

bool Event::is_event_complete() const {
    if (use_time_based_completion_) {
        const auto elapsed = std::chrono::steady_clock::now() - creation_timestamp;
        return elapsed >= std::chrono::microseconds(header.event_completion_time_us);
    }

    const int active_channels = count_active_channels(header.channel_mask);

    switch (get_trigger_type()) {
        case TriggerType::Internal: {
            const auto elapsed = std::chrono::steady_clock::now() - creation_timestamp;
            return elapsed >= std::chrono::microseconds(header.event_completion_time_us);
        }
        case TriggerType::External:
        case TriggerType::Immediate:
        case TriggerType::Unknown:
        default:
            return header.num_packets >= header.num_windows * active_channels;
    }
}

bool Event::is_event_complete(int windows,
                              const std::vector<int>& channels,
                              std::string trigger_type_str,
                              bool use_time_based_completion,
                              std::chrono::steady_clock::duration max_time_between_events) const {
    if (use_time_based_completion) {
        return (std::chrono::steady_clock::now() - creation_timestamp) >= max_time_between_events;
    }

    TriggerType trigger_type = TriggerType::Unknown;
    if (trigger_type_str == "ext") {
        trigger_type = TriggerType::External;
    } else if (trigger_type_str == "self") {
        trigger_type = TriggerType::Internal;
    } else if (trigger_type_str == "imm") {
        trigger_type = TriggerType::Immediate;
    }

    switch (trigger_type) {
        case TriggerType::Internal:
            return (std::chrono::steady_clock::now() - creation_timestamp) >= max_time_between_events;
        case TriggerType::External:
        case TriggerType::Immediate:
        case TriggerType::Unknown:
        default:
            return header.num_packets >= windows * channels.size();
    }
}

std::chrono::steady_clock::time_point Event::get_creation_timestamp() const {
    return creation_timestamp;
}

}  // namespace nalu_event_collector
