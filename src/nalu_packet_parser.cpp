#include "nalu_packet_parser.h"

#include <cstring>
#include <iostream>
#include <stdexcept>

#include "nalu_event_collector_logger.h"

NaluPacketParser::NaluPacketParser(
    size_t packet_size, const std::vector<uint8_t>& start_marker,
    const std::vector<uint8_t>& stop_marker, uint8_t chan_mask,
    uint8_t chan_shift, uint8_t abs_wind_mask, uint8_t evt_wind_mask,
    uint8_t evt_wind_shift, uint16_t timing_mask, uint8_t timing_shift,
    bool check_packet_integrity, uint16_t constructed_packet_header,
    uint16_t constructed_packet_footer)
    : packet_size(packet_size),
      chan_mask(chan_mask),
      chan_shift(chan_shift),
      abs_wind_mask(abs_wind_mask),
      evt_wind_mask(evt_wind_mask),
      evt_wind_shift(evt_wind_shift),
      timing_mask(timing_mask),
      timing_shift(timing_shift),
      start_marker(start_marker),
      stop_marker(stop_marker),
      check_packet_integrity(check_packet_integrity),
      constructed_packet_header(constructed_packet_header),
      constructed_packet_footer(constructed_packet_footer) {
    leftovers.reserve(packet_size);
}

NaluPacketParser::NaluPacketParser(const NaluPacketParserParams& params)
    : NaluPacketParser(
          params.packet_size,
          hexStringToBytes(params.start_marker),
          hexStringToBytes(params.stop_marker),
          params.chan_mask,
          params.chan_shift,
          params.abs_wind_mask,
          params.evt_wind_mask,
          params.evt_wind_shift,
          params.timing_mask,
          params.timing_shift,
          params.check_packet_integrity,
          params.constructed_packet_header,
          params.constructed_packet_footer) {}

std::vector<NaluPacket> NaluPacketParser::process_stream(
    const std::vector<UdpPacket>& udp_packets) {
    std::vector<NaluPacket> data_list;

    for (const auto& udp_packet : udp_packets) {
        const uint8_t* data_ptr = udp_packet.data.data();
        size_t byte_stream_size = udp_packet.data.size();
        size_t i = 0;
        uint8_t error_code = 0;

        size_t start_marker_len = start_marker.size();
        size_t stop_marker_len = stop_marker.size();
        size_t leftovers_size = leftovers.size();

        // Process leftovers for this UDP packet (if any)
        if (leftovers_size > 0) {
            process_leftovers(data_list, data_ptr, byte_stream_size, leftovers_size,
                              packet_size, start_marker_len, stop_marker_len,
                              udp_packet.index);
            i = packet_size - leftovers_size;
        }

        // First, process initial packets with integrity checks (to realign if needed)
        size_t prev_size = data_list.size();
        while (i + packet_size <= byte_stream_size) {
            process_byte_stream_segment_with_checks(data_list, data_ptr, i,
                                                    error_code, start_marker_len,
                                                    stop_marker_len,
                                                    udp_packet.index);
            if (data_list.size() > prev_size) {
                break;  // If we parsed one, break to normal processing
            }
        }

        // Choose processing method pointer based on integrity flag
        void (NaluPacketParser::*process_segment)(std::vector<NaluPacket>&,
                                                  const uint8_t*, size_t&, uint8_t&,
                                                  size_t, size_t, uint16_t);

        if (check_packet_integrity) {
            process_segment = &NaluPacketParser::process_byte_stream_segment_with_checks;
        } else {
            process_segment = &NaluPacketParser::process_byte_stream_segment_without_checks;
        }

        // Process remaining packets
        while (i + packet_size <= byte_stream_size) {
            (this->*process_segment)(data_list, data_ptr, i, error_code,
                                     start_marker_len, stop_marker_len,
                                     udp_packet.index);
        }

        // Store leftovers for next batch
        leftovers.clear();
        if (i < byte_stream_size) {
            leftovers.assign(data_ptr + i, data_ptr + byte_stream_size);
            leftovers_udp_packet_index = udp_packet.index;
        }
    }

    return data_list;
}

void NaluPacketParser::process_packet(std::vector<NaluPacket>& packets,
                                      const uint8_t* byte_stream,
                                      size_t start_index,
                                      uint8_t error_code,
                                      uint16_t start_udp_packet_index,
                                      uint16_t end_udp_packet_index) {
    NaluPacket p;
    p.info = error_code;
    p.header = constructed_packet_header;
    p.parser_index = packet_index++;
    packet_index %= UINT16_MAX;

    p.start_udp_packet_index = start_udp_packet_index;
    p.end_udp_packet_index = end_udp_packet_index;

    size_t j = start_index + start_marker.size();

    p.channel = byte_stream[j] & chan_mask;
    j++;

    uint16_t trigger_time_1 = (byte_stream[j] << 8) | byte_stream[j + 1];
    uint16_t trigger_time_2 = (byte_stream[j + 2] << 8) | byte_stream[j + 3];
    p.trigger_time =
        (trigger_time_1 << timing_shift) | (trigger_time_2 & timing_mask);
    j += 4;

    p.logical_position =
        ((byte_stream[j] & abs_wind_mask) << (8 - evt_wind_shift)) |
        ((byte_stream[j + 1] >> evt_wind_shift) & evt_wind_mask);
    p.physical_position = byte_stream[j + 1] & abs_wind_mask;
    j += 2;

    std::memcpy(p.raw_samples, byte_stream + j, 64);

    p.footer = constructed_packet_footer;
    packets.push_back(p);
}

bool NaluPacketParser::check_marker(const uint8_t* byte_stream, size_t index,
                                    const uint8_t* marker, size_t marker_len) {
    for (size_t j = 0; j < marker_len; ++j) {
        if (byte_stream[index + j] != marker[j]) {
            return false;
        }
    }
    return true;
}

void NaluPacketParser::process_byte_stream_segment_with_checks(
    std::vector<NaluPacket>& packets, const uint8_t* byte_stream, size_t& i,
    uint8_t& error_code, size_t start_marker_len, size_t stop_marker_len,
    uint16_t udp_packet_index) {
    size_t end_marker_pos = i + packet_size - stop_marker_len;
    if (check_marker(byte_stream, end_marker_pos, stop_marker.data(),
                     stop_marker_len)) {
        if (check_marker(byte_stream, i, start_marker.data(), start_marker_len)) {
            process_packet(packets, byte_stream, i, error_code,
                           udp_packet_index, udp_packet_index);
            error_code = 0;
            i += packet_size;
        } else {
            error_code |= 0b10;
            process_packet(packets, byte_stream, i, error_code,
                           udp_packet_index, udp_packet_index);
            i += packet_size;
            NaluEventCollectorLogger::warning("Start marker not found at expected position.");
        }
    } else {
        error_code |= 0b01;
        i++;
    }
}

void NaluPacketParser::process_byte_stream_segment_without_checks(
    std::vector<NaluPacket>& packets, const uint8_t* byte_stream, size_t& i,
    uint8_t& error_code, size_t /*start_marker_len*/, size_t /*stop_marker_len*/,
    uint16_t udp_packet_index) {
    process_packet(packets, byte_stream, i, 0, udp_packet_index, udp_packet_index);
    i += packet_size;
}

void NaluPacketParser::process_leftovers(
    std::vector<NaluPacket>& data_list, const uint8_t* byte_stream,
    size_t byte_stream_len, size_t leftovers_size, const size_t packet_size,
    const size_t start_marker_len, const size_t stop_marker_len,
    uint16_t udp_packet_index) {
    if (leftovers_size >= packet_size) {
        NaluEventCollectorLogger::warning(
            "Leftovers size (" + std::to_string(leftovers_size) +
            ") is >= packet size (" + std::to_string(packet_size) + ")");
    } else {
        size_t remaining_bytes = packet_size - leftovers_size;

        leftovers.resize(packet_size);

        std::memcpy(leftovers.data() + leftovers_size, byte_stream, remaining_bytes);

        const uint8_t* combined_ptr = leftovers.data();

        if (!check_marker(combined_ptr, 0, start_marker.data(), start_marker_len)) {
            NaluEventCollectorLogger::warning("Combined data does not start with the start marker.");
        }

        size_t end_marker_pos = packet_size - stop_marker_len;
        if (end_marker_pos <= byte_stream_len) {
            if (check_marker(combined_ptr, end_marker_pos, stop_marker.data(), stop_marker_len)) {
                process_packet(data_list, combined_ptr, 0, 0, leftovers_udp_packet_index, udp_packet_index);
            } else {
                NaluEventCollectorLogger::warning("Byte stream does not have stop marker at expected position.");
            }
        } else {
            NaluEventCollectorLogger::warning("Byte stream too short to check stop marker at expected position.");
        }
    }
}

std::vector<uint8_t> NaluPacketParser::hexStringToBytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(std::stoi(byteString, nullptr, 16));
        bytes.push_back(byte);
    }
    return bytes;
}
