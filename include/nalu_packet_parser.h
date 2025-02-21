#ifndef NALU_PACKET_PARSER_H
#define NALU_PACKET_PARSER_H

#include <vector>
#include <cstdint>
#include <iostream>
#include "nalu_packet.h"  // Including the NaluPacket struct

class NaluPacketParser {
public:
    // Constructor
    NaluPacketParser(size_t packet_size = 74, 
                     uint8_t start_marker = 0x0E, 
                     const std::vector<uint8_t>& stop_marker = {0xFA, 0x5A}, 
                     size_t scrub_length = 200, 
                     size_t error_correction_step_size = 16, 
                     size_t error_correction_max_steps = 5, 
                     const std::vector<uint8_t>& error_bytes = {0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
    
    // Getters and setters for configuration parameters
    size_t get_packet_size() const;
    void set_packet_size(size_t packet_size);
    
    uint8_t get_start_marker() const;
    void set_start_marker(uint8_t start_marker);
    
    std::vector<uint8_t> get_stop_marker() const;
    void set_stop_marker(const std::vector<uint8_t>& stop_marker);
    
    size_t get_scrub_length() const;
    void set_scrub_length(size_t scrub_length);
    
    size_t get_error_correction_step_size() const;
    void set_error_correction_step_size(size_t error_correction_step_size);
    
    size_t get_error_correction_max_steps() const;
    void set_error_correction_max_steps(size_t error_correction_max_steps);
    
    std::vector<uint8_t> get_error_bytes() const;
    void set_error_bytes(const std::vector<uint8_t>& error_bytes);

    // Method to parse packets
    std::vector<NaluPacket> parse_packet(const std::vector<std::vector<uint8_t>>& packets);

private:

    // Method to scrub the byte stream by removing error bytes
    std::vector<uint8_t> scrub_byte_stream(const std::vector<uint8_t>& byte_stream);

    // Method to handle scrubbed segment
    void handle_scrubbed_segment(std::vector<uint8_t>& scrubbed_segment, 
                                 std::vector<std::vector<uint8_t>>& packets, 
                                 size_t error_packet_start,
                                 uint8_t error_code);

    //  Method to append the error code to the packet data
    void append_error_code(std::vector<std::vector<uint8_t>>& packets, 
                           const std::vector<uint8_t>& byte_stream,
                           size_t start_index,
                           uint8_t error_code);

    // Method to split packets from byte stream
    std::vector<std::vector<uint8_t>> split_packets(const std::vector<uint8_t>& initial_byte_stream);

    size_t packet_size;
    uint8_t start_marker;
    std::vector<uint8_t> stop_marker;
    size_t scrub_length;
    size_t error_correction_step_size;
    size_t error_correction_max_steps;
    std::vector<uint8_t> error_bytes;

    uint16_t packet_index = 0; // Looping packet index, default to 0
};

#endif // NALU_PACKET_PARSER_H
