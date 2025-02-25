#ifndef NALU_PACKET_H
#define NALU_PACKET_H

#include <cstdint>
#include <cstddef>  // For size_t

class NaluPacket {
public:
    // Public fields: These will be used by other classes like NaluEvent
    uint16_t header;                  // 2-byte header
    uint8_t info;                     // 1-byte info (4 reserved bits, 4-bit error code)
    uint8_t channel;                  // 1-byte channel
    uint32_t trigger_time;            // 4-byte trigger time
    uint16_t logical_position;        // 2-byte logical position
    uint16_t physical_position;       // 2-byte physical position
    uint8_t raw_samples[64];          // 64-byte fixed-size array for raw samples
    uint16_t parser_index;            // 2-byte index tracking order of parsed packets
    uint16_t footer;                  // 2-byte footer

    // Default constructor
    NaluPacket();

    // Constructor for initialization with full info byte
    NaluPacket(uint16_t hdr, uint8_t ch, uint32_t trig_time, uint16_t log_pos, uint16_t phys_pos,
               uint8_t samples[64], uint16_t ftr, uint8_t full_info, uint16_t index = 0);

    // Method to extract the error code (lower 4 bits of info byte)
    uint8_t get_error_code() const;

    // Method to get the size of the packet in bytes
    size_t get_size() const;

private:
    // No private members needed for this class; all members are publicly accessible
};

#endif // NALU_PACKET_H
