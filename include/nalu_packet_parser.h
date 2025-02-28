#ifndef NALU_PACKET_PARSER_H
#define NALU_PACKET_PARSER_H

#include <vector>
#include <cstdint>
#include <iostream>
#include "nalu_packet.h"  // Including the NaluPacket struct
#include "nalu_event_collector_params.h"


class NaluPacketParser {
public:
    // Constructor with default values, including header and footer
    NaluPacketParser(size_t packet_size = 74, 
                     const std::vector<uint8_t>& start_marker = {0x0E}, 
                     const std::vector<uint8_t>& stop_marker = {0xFA, 0x5A},
                     uint8_t chan_mask = 0x3F, uint8_t chan_shift = 0,
                     uint8_t abs_wind_mask = 0x3F, uint8_t evt_wind_mask = 0x3F,
                     uint8_t evt_wind_shift = 6, uint16_t timing_mask = 0xFFF,
                     uint8_t timing_shift = 12,
                     bool check_packet_integrity = false,
                     uint16_t constructed_packet_header = 0xAAAA, uint16_t constructed_packet_footer = 0xFFFF);
    
    NaluPacketParser(const NaluPacketParserParams& params);

    // Getters and setters for the configuration parameters
    size_t get_packet_size() const { return packet_size; }
    void set_packet_size(size_t packet_size) { this->packet_size = packet_size; }

    uint8_t get_chan_mask() const { return chan_mask; }
    void set_chan_mask(uint8_t chan_mask) { this->chan_mask = chan_mask; }

    uint8_t get_chan_shift() const { return chan_shift; }
    void set_chan_shift(uint8_t chan_shift) { this->chan_shift = chan_shift; }

    uint8_t get_abs_wind_mask() const { return abs_wind_mask; }
    void set_abs_wind_mask(uint8_t abs_wind_mask) { this->abs_wind_mask = abs_wind_mask; }

    uint8_t get_evt_wind_mask() const { return evt_wind_mask; }
    void set_evt_wind_mask(uint8_t evt_wind_mask) { this->evt_wind_mask = evt_wind_mask; }

    uint8_t get_evt_wind_shift() const { return evt_wind_shift; }
    void set_evt_wind_shift(uint8_t evt_wind_shift) { this->evt_wind_shift = evt_wind_shift; }

    uint16_t get_timing_mask() const { return timing_mask; }
    void set_timing_mask(uint16_t timing_mask) { this->timing_mask = timing_mask; }

    uint8_t get_timing_shift() const { return timing_shift; }
    void set_timing_shift(uint8_t timing_shift) { this->timing_shift = timing_shift; }

    std::vector<uint8_t> get_start_marker() const { return start_marker; }
    void set_start_marker(const std::vector<uint8_t>& start_marker) { this->start_marker = start_marker; }

    std::vector<uint8_t> get_stop_marker() const { return stop_marker; }
    void set_stop_marker(const std::vector<uint8_t>& stop_marker) { this->stop_marker = stop_marker; }

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
    bool check_packet_integrity; 

    std::vector<uint8_t> start_marker;
    std::vector<uint8_t> stop_marker;

    uint16_t packet_index = 0;
    std::vector<uint8_t> leftovers; // Store unprocessed data

    uint16_t constructed_packet_header;
    uint16_t constructed_packet_footer;

    // Helper methods for parsing
    void process_packet(std::vector<NaluPacket>& packets, 
                        const uint8_t* byte_stream, size_t start_index,
                        uint8_t error_code);
    
    bool check_marker(const uint8_t* byte_stream, size_t index, const uint8_t* marker, size_t marker_len);

    // Private helper method: processes the byte stream segment with marker checks
    void process_byte_stream_segment_with_checks(std::vector<NaluPacket>& packets,
                                                const uint8_t* byte_stream,
                                                size_t& i, 
                                                uint8_t& error_code,
                                                size_t start_marker_len,
                                                size_t stop_marker_len);

    // Private helper method: processes the byte stream segment without marker checks
    void process_byte_stream_segment_without_checks(std::vector<NaluPacket>& packets,
                                                    const uint8_t* byte_stream,
                                                    size_t& i, 
                                                    uint8_t& error_code,
                                                    size_t start_marker_len,
                                                    size_t stop_marker_len);

    // Private helper method: processes the leftovers and byte stream
    void process_leftovers(std::vector<NaluPacket>& data_list, 
                        const uint8_t* byte_stream, 
                        size_t byte_stream_len,
                        size_t leftovers_size, 
                        const size_t packet_size,
                        const size_t start_marker_len,
                        const size_t stop_marker_len);

};

#endif // NALU_PACKET_PARSER_H
