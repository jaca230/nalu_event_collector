#ifndef NALU_PACKET_PARSER_H
#define NALU_PACKET_PARSER_H

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "nalu_event_collector_params.h"
#include "nalu_packet.h"
#include "nalu_udp_data_buffer.h"  // For UdpPacket

class NaluPacketParser {
public:
    // Constructors
    NaluPacketParser(
        size_t packet_size = 74,
        const std::vector<uint8_t>& start_marker = {0x0E},
        const std::vector<uint8_t>& stop_marker = {0xFA, 0x5A},
        uint8_t chan_mask = 0x3F, uint8_t chan_shift = 0,
        uint8_t abs_wind_mask = 0x3F, uint8_t evt_wind_mask = 0x3F,
        uint8_t evt_wind_shift = 6, uint16_t timing_mask = 0xFFF,
        uint8_t timing_shift = 12, bool check_packet_integrity = false,
        uint16_t constructed_packet_header = 0xAAAA,
        uint16_t constructed_packet_footer = 0xFFFF);

    NaluPacketParser(const NaluPacketParserParams& params);

    // Getters and setters omitted for brevity (can add if needed)

    /**
     * @brief Processes a vector of UDP packets and parses Nalu packets.
     * @param udp_packets Vector of UDP packets (with index and bytes).
     * @return Vector of parsed NaluPacket objects.
     */
    std::vector<NaluPacket> process_stream(const std::vector<UdpPacket>& udp_packets);

private:
    size_t packet_size;
    uint8_t chan_mask;
    uint8_t chan_shift;
    uint8_t abs_wind_mask;
    uint8_t evt_wind_mask;
    uint8_t evt_wind_shift;
    uint16_t timing_mask;
    uint8_t timing_shift;
    bool check_packet_integrity;
    std::vector<uint8_t> start_marker;
    std::vector<uint8_t> stop_marker;

    uint16_t packet_index = 0;
    std::vector<uint8_t> leftovers;
    uint16_t leftovers_udp_packet_index = 0;
    uint16_t constructed_packet_header;
    uint16_t constructed_packet_footer;

    // Internal helpers
    void process_packet(std::vector<NaluPacket>& packets, const uint8_t* byte_stream,
                        size_t start_index, uint8_t error_code,
                        uint16_t start_udp_packet_index,
                        uint16_t end_udp_packet_index);

    bool check_marker(const uint8_t* byte_stream, size_t index,
                      const uint8_t* marker, size_t marker_len);

    void process_byte_stream_segment_with_checks(
        std::vector<NaluPacket>& packets, const uint8_t* byte_stream, size_t& i,
        uint8_t& error_code, size_t start_marker_len, size_t stop_marker_len,
        uint16_t udp_packet_index);

    void process_byte_stream_segment_without_checks(
        std::vector<NaluPacket>& packets, const uint8_t* byte_stream, size_t& i,
        uint8_t& error_code, size_t start_marker_len, size_t stop_marker_len,
        uint16_t udp_packet_index);

    void process_leftovers(std::vector<NaluPacket>& data_list,
                           const uint8_t* byte_stream, size_t byte_stream_len,
                           size_t leftovers_size, const size_t packet_size,
                           const size_t start_marker_len, const size_t stop_marker_len,
                           uint16_t udp_packet_index);

    std::vector<uint8_t> hexStringToBytes(const std::string& hex);
};

#endif  // NALU_PACKET_PARSER_H
