/**
 * @file packet_parser_config.h
 * @brief Configuration for raw byte-stream packet parsing.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace nalu_event_collector {

/**
 * @brief Configuration for constructing a PacketParser.
 */
struct PacketParserConfig {
    /** @brief Packet size in bytes before conversion into Packet objects. */
    size_t packet_size = 74;

    /** @brief Start marker encoded as a hexadecimal string. */
    std::string start_marker = "0E";

    /** @brief Stop marker encoded as a hexadecimal string. */
    std::string stop_marker = "FA5A";

    /** @brief Mask applied while decoding channel identifiers. */
    uint8_t chan_mask = 0x3F;

    /** @brief Shift applied before channel masking. */
    uint8_t chan_shift = 0;

    /** @brief Mask used while decoding absolute window state. */
    uint8_t abs_wind_mask = 0x3F;

    /** @brief Mask used while decoding event-local window state. */
    uint8_t evt_wind_mask = 0x3F;

    /** @brief Shift used while decoding event-local window state. */
    uint8_t evt_wind_shift = 6;

    /** @brief Mask used while decoding trigger timing information. */
    uint16_t timing_mask = 0xFFF;

    /** @brief Shift used while decoding trigger timing information. */
    uint8_t timing_shift = 12;

    /** @brief Enable explicit marker validation while parsing. */
    bool check_packet_integrity = false;

    /** @brief Synthetic packet header written into parsed Packet objects. */
    uint16_t constructed_packet_header = 0xAAAA;

    /** @brief Synthetic packet footer written into parsed Packet objects. */
    uint16_t constructed_packet_footer = 0xFFFF;
};

}  // namespace nalu_event_collector
