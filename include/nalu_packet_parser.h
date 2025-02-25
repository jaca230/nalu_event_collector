#ifndef NALU_PACKET_PARSER_H
#define NALU_PACKET_PARSER_H

#include <vector>
#include <cstdint>
#include <iostream>
#include "nalu_packet.h"  // Including the NaluPacket struct

class NaluPacketParser {
public:
    // Constructor with default values
    NaluPacketParser(size_t packet_size = 74, 
                     const std::vector<uint8_t>& start_marker = {0x0E}, 
                     const std::vector<uint8_t>& stop_marker = {0xFA, 0x5A},
                     uint8_t chan_mask = 0x3F, uint8_t chan_shift = 0,
                     uint8_t abs_wind_mask = 0x3F, uint8_t evt_wind_mask = 0x3F,
                     uint8_t evt_wind_shift = 6, uint16_t timing_mask = 0xFFF,
                     uint8_t timing_shift = 12);

    // Getters and setters for the configuration parameters
    size_t get_packet_size() const;
    void set_packet_size(size_t packet_size);

    uint8_t get_chan_mask() const;
    void set_chan_mask(uint8_t chan_mask);

    uint8_t get_chan_shift() const;
    void set_chan_shift(uint8_t chan_shift);

    uint8_t get_abs_wind_mask() const;
    void set_abs_wind_mask(uint8_t abs_wind_mask);

    uint8_t get_evt_wind_mask() const;
    void set_evt_wind_mask(uint8_t evt_wind_mask);

    uint8_t get_evt_wind_shift() const;
    void set_evt_wind_shift(uint8_t evt_wind_shift);

    uint16_t get_timing_mask() const;
    void set_timing_mask(uint16_t timing_mask);

    uint8_t get_timing_shift() const;
    void set_timing_shift(uint8_t timing_shift);

    std::vector<uint8_t> get_start_marker() const;
    void set_start_marker(const std::vector<uint8_t>& start_marker);

    std::vector<uint8_t> get_stop_marker() const;
    void set_stop_marker(const std::vector<uint8_t>& stop_marker);

    // Method to process the byte stream and parse packets
    std::vector<NaluPacket> process_stream(const std::vector<uint8_t>& byte_stream);

private:
    size_t packet_size;
    uint8_t chan_mask;
    uint8_t chan_shift;
    uint8_t abs_wind_mask;
    uint8_t evt_wind_mask;
    uint8_t evt_wind_shift;
    uint16_t timing_mask;
    uint8_t timing_shift;
    bool raw;

    std::vector<uint8_t> start_marker;
    std::vector<uint8_t> stop_marker;

    uint16_t packet_index = 0; // Looping packet index, default to 0
    std::vector<uint8_t> leftovers; // Store unprocessed data

    // Helper methods for parsing
    void process_packet(std::vector<NaluPacket>& packets, 
                        const std::vector<uint8_t>& byte_stream,
                        size_t start_index,
                        uint8_t error_code);

    bool check_marker(const std::vector<uint8_t>& byte_stream, size_t index, const std::vector<uint8_t>& marker);
};

#endif // NALU_PACKET_PARSER_H
