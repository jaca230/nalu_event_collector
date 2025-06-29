#include "nalu_event.h"

#include <chrono>
#include <cstring>  // For std::memcpy
#include <iostream>
#include <stdexcept>

#include "nalu_event_collector_logger.h"

// Default constructor
NaluEvent::NaluEvent()
    : header{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      footer{0},
      max_packets(0),
      packets(nullptr) {
    creation_timestamp = std::chrono::steady_clock::now();
}

// Constructor with parameters (including time_threshold and clock_frequency)
NaluEvent::NaluEvent(uint16_t hdr, uint8_t extra_info, uint32_t idx,
                     uint32_t ref_time, uint32_t time_thresh, uint32_t clk_freq,
                     uint16_t size, uint16_t num,
                     uint16_t ftr, uint16_t max_num_packets,
                     uint64_t channel_mask_value, uint8_t num_windows_value)
    : header{hdr, extra_info, idx, ref_time, time_thresh, clk_freq, size,
             channel_mask_value, num_windows_value, num},
      footer{ftr},
      max_packets(max_num_packets),
      packets(std::make_unique<NaluPacket[]>(max_num_packets)) {
    creation_timestamp = std::chrono::steady_clock::now();
}

// Print event info (header + footer)
void NaluEvent::print_event_info() const {
    std::cout << "Event Header: " << std::endl;
    std::cout << "Header: " << header.header << std::endl;
    std::cout << "Info: " << static_cast<int>(header.info) << std::endl;
    std::cout << "Index: " << header.index << std::endl;
    std::cout << "Reference Time: " << header.reference_time << std::endl;
    std::cout << "Time Threshold (ticks): " << header.time_threshold << std::endl;
    std::cout << "Clock Frequency (Hz): " << header.clock_frequency << std::endl;
    std::cout << "Packet Size: " << header.packet_size << std::endl;
    std::cout << "Channel Mask: " << header.channel_mask << std::endl;
    std::cout << "Number of Windows: " << static_cast<int>(header.num_windows) << std::endl;
    std::cout << "Number of Packets: " << header.num_packets << std::endl;

    std::cout << "Event Footer: " << std::endl;
    std::cout << "Footer: " << footer.footer << std::endl;
}

// Calculate size of event (header + packets + footer)
size_t NaluEvent::get_size() const {
    size_t total_size = sizeof(header) + sizeof(footer);
    total_size += header.num_packets * sizeof(NaluPacket);
    return total_size;
}

// Extract last 4 bits of info as error code
uint8_t NaluEvent::get_error_code() const {
    return header.info & 0x0F;
}

// Extract trigger type from bits 4 and 5
NaluEvent::TriggerType NaluEvent::get_trigger_type() const {
    uint8_t trigger_bits = (header.info & 0x30) >> 4;
    switch (trigger_bits) {
        case 1: return TriggerType::External;
        case 2: return TriggerType::Internal;
        case 3: return TriggerType::Immediate;
        default: return TriggerType::Unknown;
    }
}

// Serialize event into buffer
void NaluEvent::serialize_to_buffer(char* buffer) const {
    if (!buffer) return;
    size_t offset = 0;
    std::memcpy(buffer + offset, &header, sizeof(header));
    offset += sizeof(header);
    std::memcpy(buffer + offset, packets.get(), header.num_packets * sizeof(NaluPacket));
    offset += header.num_packets * sizeof(NaluPacket);
    std::memcpy(buffer + offset, &footer, sizeof(footer));
}

// Add packet (throws if max_packets exceeded)
void NaluEvent::add_packet(const NaluPacket& packet) {
    if (header.num_packets < max_packets) {
        packets[header.num_packets] = packet;
        header.num_packets++;
    } else {
        NaluEventCollectorLogger::error(
            "Attempt to add packet exceeds max packet limit. Max: " +
            std::to_string(max_packets) + ", Current: " +
            std::to_string(header.num_packets));
        throw std::overflow_error("Maximum number of packets exceeded.");
    }
}

// Count active channels in channel mask
int NaluEvent::count_active_channels(uint64_t channel_mask) const {
    int count = 0;
    while (channel_mask) {
        count += channel_mask & 1;
        channel_mask >>= 1;
    }
    return count;
}

// Check if event is complete using embedded time_threshold and clock_frequency
bool NaluEvent::is_event_complete() const {
    int active_channels = count_active_channels(header.channel_mask);
    TriggerType trigger_type = get_trigger_type();

    switch (trigger_type) {
        case TriggerType::Internal: {
            auto elapsed = std::chrono::steady_clock::now() - creation_timestamp;

            // Convert ticks to duration using embedded clock frequency
            double ns = static_cast<double>(header.time_threshold) * (1e9 / header.clock_frequency);
            auto threshold_duration = std::chrono::nanoseconds(static_cast<int64_t>(ns));
            return elapsed >= threshold_duration;
        }
        case TriggerType::External:
        case TriggerType::Immediate:
        case TriggerType::Unknown:
        default:
            return header.num_packets >= header.num_windows * active_channels;
    }
}

// Alternative event completion check with external parameters (kept for compatibility)
bool NaluEvent::is_event_complete(int windows,
                                  const std::vector<int>& channels,
                                  std::string trigger_type_str,
                                  std::chrono::steady_clock::duration max_time_between_events) const {
    TriggerType trigger_type = TriggerType::Unknown;
    if (trigger_type_str == "ext") trigger_type = TriggerType::External;
    else if (trigger_type_str == "self") trigger_type = TriggerType::Internal;
    else if (trigger_type_str == "imm") trigger_type = TriggerType::Immediate;

    switch (trigger_type) {
        case TriggerType::Internal: {
            auto elapsed = std::chrono::steady_clock::now() - creation_timestamp;
            return elapsed >= max_time_between_events;
        }
        case TriggerType::External:
        case TriggerType::Immediate:
        case TriggerType::Unknown:
        default:
            return header.num_packets >= windows * channels.size();
    }
}

// Get event creation timestamp
std::chrono::steady_clock::time_point NaluEvent::get_creation_timestamp() const {
    return creation_timestamp;
}
