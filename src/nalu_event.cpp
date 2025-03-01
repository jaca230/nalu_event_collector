#include "nalu_event.h"
#include "nalu_event_collector_logger.h"
#include <cstring>  // For std::memcpy
#include <stdexcept>  // For exceptions
#include <iostream>
#include <chrono>

NaluEvent::NaluEvent()
    : header(0), info(0), index(0), reference_time(0), packet_size(0), num_packets(0), packets(nullptr), footer(0), max_packets(0) {
    // Initialize creation timestamp to current time
    creation_timestamp = std::chrono::steady_clock::now();
}

NaluEvent::NaluEvent(uint16_t hdr, uint16_t extra_info, uint32_t idx, uint32_t ref_time, uint16_t size, uint16_t num, uint16_t ftr, uint16_t max_num_packets)
    : header(hdr), info(extra_info), index(idx), reference_time(ref_time), packet_size(size), num_packets(num), footer(ftr), max_packets(max_num_packets) {
    packets = std::make_unique<NaluPacket[]>(max_num_packets);  // Allocate memory for packets using unique_ptr
    // Initialize creation timestamp to current time
    creation_timestamp = std::chrono::steady_clock::now();
}

// Calculate the size of the entire NaluEvent for serialization
size_t NaluEvent::get_size() const {
    size_t total_size = sizeof(header) + sizeof(info) + sizeof(index) + sizeof(reference_time) +
                        sizeof(packet_size) + sizeof(num_packets) + sizeof(footer);
    total_size += num_packets * sizeof(NaluPacket);
    return total_size;
}

uint8_t NaluEvent::get_error_code() const {
    return info & 0x0F;  // Mask the last 4 bits
}

// Serialize this object to a buffer
void NaluEvent::serialize_to_buffer(char* buffer) const {
    if (!buffer) return;

    size_t offset = 0;

    // Copy all header fields (everything except packets)
    std::memcpy(buffer + offset, this, offsetof(NaluEvent, packets));  // Up to packets
    offset += offsetof(NaluEvent, packets);

    // Copy packet data (can be large, but it's contiguous memory)
    std::memcpy(buffer + offset, packets.get(), num_packets * sizeof(NaluPacket));
    offset += num_packets * sizeof(NaluPacket);

    // Copy footer
    std::memcpy(buffer + offset, &footer, sizeof(footer));
}

// Add a packet to the event (if space allows)
void NaluEvent::add_packet(const NaluPacket& packet) {
    if (num_packets < max_packets) {
        packets[num_packets] = packet;  // Add the packet
        num_packets++;  // Increment counter
    } else {
        // Log error before throwing exception
        NaluEventCollectorLogger::error("Attempt to add packet exceeds maximum packet limit. Max packets: " + std::to_string(max_packets) + ", Current count: " + std::to_string(num_packets));
        throw std::overflow_error("Maximum number of packets exceeded.");
    }
}

// Check if the event is complete based on windows and channels
bool NaluEvent::is_event_complete(int windows, const std::vector<int>& channels) const {
    return num_packets >= windows * channels.size();
}

// Getter for the creation timestamp
std::chrono::steady_clock::time_point NaluEvent::get_creation_timestamp() const {
    return creation_timestamp;
}
