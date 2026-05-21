/**
 * @file packet_parser.h
 * @brief Byte-stream parser for constructing Packet objects from raw payloads.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "nalu_event_collector/config/packet_parser_config.h"
#include "nalu_event_collector/data/packet.h"

namespace nalu_event_collector {

/**
 * @brief Incrementally parses a raw byte stream into Packet objects.
 *
 * The parser supports optional marker validation, carries partial-packet
 * leftovers across calls, and converts hardware byte fields into the collector
 * packet model.
 */
class PacketParser {
  public:
    /** @brief Construct a parser from explicit decoding parameters. */
    PacketParser(size_t packet_size = 74,
                 const std::vector<uint8_t>& start_marker = {0x0E},
                 const std::vector<uint8_t>& stop_marker = {0xFA, 0x5A},
                 uint8_t chan_mask = 0x3F,
                 uint8_t chan_shift = 0,
                 uint8_t abs_wind_mask = 0x3F,
                 uint8_t evt_wind_mask = 0x3F,
                 uint8_t evt_wind_shift = 6,
                 uint16_t timing_mask = 0xFFF,
                 uint8_t timing_shift = 12,
                 bool check_packet_integrity = false,
                 uint16_t constructed_packet_header = 0xAAAA,
                 uint16_t constructed_packet_footer = 0xFFFF);

    /** @brief Construct a parser from a configuration object. */
    explicit PacketParser(const PacketParserConfig& config);

    /** @brief Return the configured raw packet size. */
    size_t get_packet_size() const;

    /** @brief Set the configured raw packet size. */
    void set_packet_size(size_t packet_size);

    /** @brief Return the channel mask used during decoding. */
    uint8_t get_chan_mask() const;

    /** @brief Set the channel mask used during decoding. */
    void set_chan_mask(uint8_t chan_mask);

    /** @brief Return the channel shift used during decoding. */
    uint8_t get_chan_shift() const;

    /** @brief Set the channel shift used during decoding. */
    void set_chan_shift(uint8_t chan_shift);

    /** @brief Return the absolute-window mask used during decoding. */
    uint8_t get_abs_wind_mask() const;

    /** @brief Set the absolute-window mask used during decoding. */
    void set_abs_wind_mask(uint8_t abs_wind_mask);

    /** @brief Return the event-window mask used during decoding. */
    uint8_t get_evt_wind_mask() const;

    /** @brief Set the event-window mask used during decoding. */
    void set_evt_wind_mask(uint8_t evt_wind_mask);

    /** @brief Return the event-window shift used during decoding. */
    uint8_t get_evt_wind_shift() const;

    /** @brief Set the event-window shift used during decoding. */
    void set_evt_wind_shift(uint8_t evt_wind_shift);

    /** @brief Return the timing mask used during decoding. */
    uint16_t get_timing_mask() const;

    /** @brief Set the timing mask used during decoding. */
    void set_timing_mask(uint16_t timing_mask);

    /** @brief Return the timing shift used during decoding. */
    uint8_t get_timing_shift() const;

    /** @brief Set the timing shift used during decoding. */
    void set_timing_shift(uint8_t timing_shift);

    /** @brief Return the start marker bytes. */
    std::vector<uint8_t> get_start_marker() const;

    /** @brief Set the start marker bytes. */
    void set_start_marker(const std::vector<uint8_t>& start_marker);

    /** @brief Return the stop marker bytes. */
    std::vector<uint8_t> get_stop_marker() const;

    /** @brief Set the stop marker bytes. */
    void set_stop_marker(const std::vector<uint8_t>& stop_marker);

    /** @brief Parse @p byte_stream into zero or more Packet objects. */
    std::vector<Packet> process_stream(const std::vector<uint8_t>& byte_stream);

  private:
    static std::vector<uint8_t> hexStringToBytes(const std::string& hex);

    void process_packet(std::vector<Packet>& packets,
                        const uint8_t* byte_stream,
                        size_t start_index,
                        uint8_t error_code);
    bool check_marker(const uint8_t* byte_stream,
                      size_t index,
                      const uint8_t* marker,
                      size_t marker_len);
    void process_byte_stream_segment_with_checks(std::vector<Packet>& packets,
                                                 const uint8_t* byte_stream,
                                                 size_t& i,
                                                 uint8_t& error_code,
                                                 size_t start_marker_len,
                                                 size_t stop_marker_len);
    void process_byte_stream_segment_without_checks(std::vector<Packet>& packets,
                                                    const uint8_t* byte_stream,
                                                    size_t& i,
                                                    uint8_t& error_code,
                                                    size_t start_marker_len,
                                                    size_t stop_marker_len);
    void process_leftovers(std::vector<Packet>& data_list,
                           const uint8_t* byte_stream,
                           size_t byte_stream_len,
                           size_t leftovers_size,
                           size_t packet_size,
                           size_t start_marker_len,
                           size_t stop_marker_len);

    size_t packet_size_;
    uint8_t chan_mask_;
    uint8_t chan_shift_;
    uint8_t abs_wind_mask_;
    uint8_t evt_wind_mask_;
    uint8_t evt_wind_shift_;
    uint16_t timing_mask_;
    uint8_t timing_shift_;
    bool check_packet_integrity_;
    std::vector<uint8_t> start_marker_;
    std::vector<uint8_t> stop_marker_;
    uint16_t packet_index_ = 0;
    std::vector<uint8_t> leftovers_;
    uint16_t constructed_packet_header_;
    uint16_t constructed_packet_footer_;
};

}  // namespace nalu_event_collector
