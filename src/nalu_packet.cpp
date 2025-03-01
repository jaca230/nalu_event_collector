#include "nalu_packet.h"
#include <cstring>  // For std::memcpy

// Default constructor
NaluPacket::NaluPacket() 
    : header(0), info(0), channel(0), trigger_time(0), logical_position(0),
      physical_position(0), footer(0), parser_index(0) {
    std::memset(raw_samples, 0, sizeof(raw_samples));  // Initialize raw_samples to 0
}

// Constructor for initialization with full info byte
NaluPacket::NaluPacket(uint16_t hdr, uint8_t ch, uint32_t trig_time, uint16_t log_pos, uint16_t phys_pos,
                       uint8_t samples[64], uint16_t ftr, uint8_t full_info, uint16_t index)
    : header(hdr), channel(ch), trigger_time(trig_time),
      logical_position(log_pos), physical_position(phys_pos),
      footer(ftr), info(full_info), parser_index(index) {
    // Directly assign the raw_samples array
    std::memcpy(raw_samples, samples, sizeof(raw_samples));
}

// Method to extract the error code (lower 4 bits of info byte)
uint8_t NaluPacket::get_error_code() const {
    return info & 0x0F; // Extract lower 4 bits for error code
}

// Method to get the size of the packet in bytes
uint16_t NaluPacket::get_size() const {
    return sizeof(header) + sizeof(channel) + sizeof(trigger_time) +
           sizeof(logical_position) + sizeof(physical_position) +
           sizeof(raw_samples) + sizeof(footer) + sizeof(info) +
           sizeof(parser_index);
}
