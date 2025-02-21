#ifndef NALU_EVENT_H
#define NALU_EVENT_H

#include <cstdint>
#include <cstring>  // For std::memcpy
#include "nalu_packet.h"

struct NaluEvent {
    uint16_t header;          // 2-byte header
    uint8_t info;             // 1-byte info (4 reserved bits, 4-bit error code)
    uint32_t index;           // 4-byte index
    uint32_t reference_time;  // 4-byte reference time
    uint8_t packet_size;      // 1-byte specifying packet size (in bytes)
    uint16_t num_packets;     // 2-byte number of packets in event
    NaluPacket* packets;      // Pointer to dynamically allocated array for NaluPackets
    uint16_t footer;          // 2-byte footer

    // Default constructor
    NaluEvent() : header(0), info(0), index(0), reference_time(0), packet_size(0), 
                  num_packets(0), packets(nullptr), footer(0) {}

    // Constructor for initializing the event
    NaluEvent(uint16_t hdr, uint8_t extra_info, uint32_t idx, uint32_t ref_time, 
              uint8_t size, uint16_t num, uint16_t ftr, uint16_t max_num_packets)
        : header(hdr), info(extra_info), index(idx), reference_time(ref_time), 
          packet_size(size), num_packets(num), footer(ftr) {
        
        // Dynamically allocate memory for the packets array
        packets = new NaluPacket[max_num_packets];
    }

    // Destructor to release dynamically allocated memory
    ~NaluEvent() {
        delete[] packets;
    }

    // Extract the error code (lower 4 bits of info byte)
    uint8_t get_error_code() const {
        return info & 0x0F; // Extract lower 4 bits for error code
    }

    // Get the size of the event in bytes
    size_t get_size() const {
        size_t total_size = sizeof(header) + sizeof(info) + sizeof(index) + sizeof(reference_time) +
                            sizeof(packet_size) + sizeof(num_packets) + sizeof(footer);
        total_size += num_packets * sizeof(NaluPacket); // Size of the list of packets
        return total_size;
    }
};

#endif // NALU_EVENT_H
