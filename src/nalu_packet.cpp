#include "nalu_packet.h"

#include <cstring>  // For std::memcpy

// Default constructor
NaluPacket::NaluPacket()
    : header(0),
      info(0),
      channel(0),
      trigger_time(0),
      logical_position(0),
      physical_position(0),
      parser_index(0),
      start_udp_packet_index(0),
      end_udp_packet_index(0),
      footer(0) {
    std::memset(raw_samples, 0, sizeof(raw_samples));
}

// Constructor with full initialization
NaluPacket::NaluPacket(uint16_t hdr, uint8_t ch, uint32_t trig_time,
                       uint16_t log_pos, uint16_t phys_pos, uint8_t samples[64],
                       uint16_t ftr, uint8_t full_info, uint16_t index,
                       uint16_t start_idx, uint16_t end_idx)
    : header(hdr),
      info(full_info),
      channel(ch),
      trigger_time(trig_time),
      logical_position(log_pos),
      physical_position(phys_pos),
      parser_index(index),
      start_udp_packet_index(start_idx),
      end_udp_packet_index(end_idx),
      footer(ftr) {
    std::memcpy(raw_samples, samples, sizeof(raw_samples));
}

// Extracts the lower 4 bits (error code) from the info byte
uint8_t NaluPacket::get_error_code() const {
    return info & 0x0F;
}

// Returns the total size of the packet in bytes
uint16_t NaluPacket::get_size() const {
    return sizeof(header) +
           sizeof(info) +
           sizeof(channel) +
           sizeof(trigger_time) +
           sizeof(logical_position) +
           sizeof(physical_position) +
           sizeof(raw_samples) +
           sizeof(parser_index) +
           sizeof(start_udp_packet_index) +
           sizeof(end_udp_packet_index) +
           sizeof(footer);
}

// Prints a summary of the packet's contents
void NaluPacket::printout() const {
    std::cout << "Nalu Packet Details:" << std::endl;
    std::cout << "Header: 0x" << std::hex << header << std::dec << std::endl;
    std::cout << "Info Byte: 0x" << std::hex << (int)info << std::dec << std::endl;
    std::cout << "Channel: " << (int)channel << std::endl;
    std::cout << "Trigger Time: " << trigger_time << std::endl;
    std::cout << "Logical Position: " << logical_position << std::endl;
    std::cout << "Physical Position: " << physical_position << std::endl;
    std::cout << "Parser Index: " << parser_index << std::endl;
    std::cout << "Start UDP Packet Index: " << start_udp_packet_index << std::endl;
    std::cout << "End UDP Packet Index: " << end_udp_packet_index << std::endl;
    std::cout << "Footer: 0x" << std::hex << footer << std::dec << std::endl;
    std::cout << "Error Code: 0x" << std::hex << (int)get_error_code() << std::dec << std::endl;

    std::cout << "Raw Samples (first 10 bytes): ";
    for (int i = 0; i < 10; ++i) {
        std::cout << std::hex << (int)raw_samples[i] << " ";
    }
    std::cout << std::dec << std::endl;
}
