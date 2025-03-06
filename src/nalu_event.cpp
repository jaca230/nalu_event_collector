#include "nalu_event.h"

#include <chrono>
#include <cstring>  // For std::memcpy
#include <iostream>
#include <stdexcept>  // For exceptions

#include "nalu_event_collector_logger.h"

// Default constructor
NaluEvent::NaluEvent()
    : header{0, 0, 0, 0, 0, 0, 0, 0},
      footer{0},
      max_packets(0),
      packets(nullptr) {
    // Initialize creation timestamp to current time
    creation_timestamp = std::chrono::steady_clock::now();
}

// Constructor with specific values
NaluEvent::NaluEvent(uint16_t hdr, uint8_t extra_info, uint32_t idx,
                     uint32_t ref_time, uint16_t size, uint16_t num,
                     uint16_t ftr, uint16_t max_num_packets,
                     uint64_t channel_mask_value, uint8_t num_windows_value)
    : header{hdr, extra_info, idx, ref_time, size, channel_mask_value, num_windows_value, num},
      footer{ftr},
      max_packets(max_num_packets),
      packets(std::make_unique<NaluPacket[]>(max_num_packets)) {
    // Initialize creation timestamp to current time
    creation_timestamp = std::chrono::steady_clock::now();
}

// Calculate the size of the entire NaluEvent for serialization
size_t NaluEvent::get_size() const {
    size_t total_size = sizeof(header) + sizeof(footer); // header + footer size
    total_size += header.num_packets * sizeof(NaluPacket); // Adding packet sizes
    return total_size;
}

// Get the error code from the info field (last 4 bits)
uint8_t NaluEvent::get_error_code() const {
    return header.info & 0x0F;  // Mask the last 4 bits
}

// Serialize the event into a buffer
void NaluEvent::serialize_to_buffer(char* buffer) const {
    if (!buffer) return;

    size_t offset = 0;

    // Copy header struct
    std::memcpy(buffer + offset, &header, sizeof(header));
    offset += sizeof(header);

    // Copy packet data (can be large, but it's contiguous memory)
    std::memcpy(buffer + offset, packets.get(), header.num_packets * sizeof(NaluPacket));
    offset += header.num_packets * sizeof(NaluPacket);

    // Copy footer
    std::memcpy(buffer + offset, &footer, sizeof(footer));
}

// Add a packet to the event (if space allows)
void NaluEvent::add_packet(const NaluPacket& packet) {
    if (header.num_packets < max_packets) {
        packets[header.num_packets] = packet;  // Add the packet
        header.num_packets++;                  // Increment counter
    } else {
        // Log error before throwing exception
        NaluEventCollectorLogger::error(
            "Attempt to add packet exceeds maximum packet limit. Max "
            "packets: " +
            std::to_string(max_packets) +
            ", Current count: " + std::to_string(header.num_packets));
        throw std::overflow_error("Maximum number of packets exceeded.");
    }
}

// Check if the event is complete based on internal information (overloaded)
bool NaluEvent::is_event_complete() const {
    int active_channels = count_active_channels(header.channel_mask);
    return header.num_packets >= header.num_windows * active_channels;
}

// Check if the event is complete based on windows and channels
bool NaluEvent::is_event_complete(int windows,
    const std::vector<int>& channels) const {
    return header.num_packets >= windows * channels.size();
}

// Getter for the creation timestamp
std::chrono::steady_clock::time_point NaluEvent::get_creation_timestamp() const {
    return creation_timestamp;
}


int NaluEvent::count_active_channels(uint64_t channel_mask) const {
    int count = 0;
    while (channel_mask) {
        count += channel_mask & 1;
        channel_mask >>= 1;
    }
    return count;
}