/**
 * @file packet_parser.cpp
 * @brief Implements incremental byte-stream parsing into Packet objects.
 */

#include "nalu_event_collector/parsing/packet_parser.h"

#include <cstring>
#include <stdexcept>

#include <spdlog/spdlog.h>

namespace nalu_event_collector {

PacketParser::PacketParser(size_t packet_size,
                           const std::vector<uint8_t>& start_marker,
                           const std::vector<uint8_t>& stop_marker,
                           uint8_t chan_mask,
                           uint8_t chan_shift,
                           uint8_t abs_wind_mask,
                           uint8_t evt_wind_mask,
                           uint8_t evt_wind_shift,
                           uint16_t timing_mask,
                           uint8_t timing_shift,
                           bool check_packet_integrity,
                           uint16_t constructed_packet_header,
                           uint16_t constructed_packet_footer)
    : packet_size_(packet_size),
      chan_mask_(chan_mask),
      chan_shift_(chan_shift),
      abs_wind_mask_(abs_wind_mask),
      evt_wind_mask_(evt_wind_mask),
      evt_wind_shift_(evt_wind_shift),
      timing_mask_(timing_mask),
      timing_shift_(timing_shift),
      check_packet_integrity_(check_packet_integrity),
      start_marker_(start_marker),
      stop_marker_(stop_marker),
      constructed_packet_header_(constructed_packet_header),
      constructed_packet_footer_(constructed_packet_footer) {
    leftovers_.reserve(packet_size_);
}

PacketParser::PacketParser(const PacketParserConfig& config)
    : PacketParser(config.packet_size,
                   hexStringToBytes(config.start_marker),
                   hexStringToBytes(config.stop_marker),
                   config.chan_mask,
                   config.chan_shift,
                   config.abs_wind_mask,
                   config.evt_wind_mask,
                   config.evt_wind_shift,
                   config.timing_mask,
                   config.timing_shift,
                   config.check_packet_integrity,
                   config.constructed_packet_header,
                   config.constructed_packet_footer) {}

size_t PacketParser::get_packet_size() const { return packet_size_; }
void PacketParser::set_packet_size(size_t packet_size) { packet_size_ = packet_size; }
uint8_t PacketParser::get_chan_mask() const { return chan_mask_; }
void PacketParser::set_chan_mask(uint8_t chan_mask) { chan_mask_ = chan_mask; }
uint8_t PacketParser::get_chan_shift() const { return chan_shift_; }
void PacketParser::set_chan_shift(uint8_t chan_shift) { chan_shift_ = chan_shift; }
uint8_t PacketParser::get_abs_wind_mask() const { return abs_wind_mask_; }
void PacketParser::set_abs_wind_mask(uint8_t abs_wind_mask) { abs_wind_mask_ = abs_wind_mask; }
uint8_t PacketParser::get_evt_wind_mask() const { return evt_wind_mask_; }
void PacketParser::set_evt_wind_mask(uint8_t evt_wind_mask) { evt_wind_mask_ = evt_wind_mask; }
uint8_t PacketParser::get_evt_wind_shift() const { return evt_wind_shift_; }
void PacketParser::set_evt_wind_shift(uint8_t evt_wind_shift) { evt_wind_shift_ = evt_wind_shift; }
uint16_t PacketParser::get_timing_mask() const { return timing_mask_; }
void PacketParser::set_timing_mask(uint16_t timing_mask) { timing_mask_ = timing_mask; }
uint8_t PacketParser::get_timing_shift() const { return timing_shift_; }
void PacketParser::set_timing_shift(uint8_t timing_shift) { timing_shift_ = timing_shift; }
std::vector<uint8_t> PacketParser::get_start_marker() const { return start_marker_; }
void PacketParser::set_start_marker(const std::vector<uint8_t>& start_marker) { start_marker_ = start_marker; }
std::vector<uint8_t> PacketParser::get_stop_marker() const { return stop_marker_; }
void PacketParser::set_stop_marker(const std::vector<uint8_t>& stop_marker) { stop_marker_ = stop_marker; }

std::vector<Packet> PacketParser::process_stream(const std::vector<uint8_t>& byte_stream) {
    std::vector<Packet> packets;
    const uint8_t* data_ptr = byte_stream.data();
    uint8_t error_code = 0;
    const size_t stop_marker_len = stop_marker_.size();
    const size_t start_marker_len = start_marker_.size();
    const size_t leftovers_size = leftovers_.size();
    size_t i = 0;

    if (leftovers_size > 0) {
        process_leftovers(packets,
                          data_ptr,
                          byte_stream.size(),
                          leftovers_size,
                          packet_size_,
                          start_marker_len,
                          stop_marker_len);
        i = packet_size_ - leftovers_size;
    }

    const size_t initial_packets = packets.size();
    while (i + packet_size_ <= byte_stream.size()) {
        process_byte_stream_segment_with_checks(
            packets, data_ptr, i, error_code, start_marker_len, stop_marker_len);
        if (packets.size() > initial_packets) {
            break;
        }
    }

    auto process_segment = check_packet_integrity_
                               ? &PacketParser::process_byte_stream_segment_with_checks
                               : &PacketParser::process_byte_stream_segment_without_checks;

    while (i + packet_size_ <= byte_stream.size()) {
        (this->*process_segment)(
            packets, data_ptr, i, error_code, start_marker_len, stop_marker_len);
    }

    leftovers_.clear();
    if (i < byte_stream.size()) {
        leftovers_.assign(byte_stream.begin() + i, byte_stream.end());
    }

    return packets;
}

void PacketParser::process_packet(std::vector<Packet>& packets,
                                  const uint8_t* byte_stream,
                                  size_t start_index,
                                  uint8_t error_code) {
    Packet packet;
    packet.info = error_code;
    packet.header = constructed_packet_header_;
    packet.parser_index = packet_index_++;
    packet_index_ %= UINT16_MAX;

    size_t j = start_index + start_marker_.size();
    packet.channel = static_cast<uint8_t>((byte_stream[j] >> chan_shift_) & chan_mask_);
    ++j;

    const uint16_t trigger_time_1 = static_cast<uint16_t>((byte_stream[j] << 8) | byte_stream[j + 1]);
    const uint16_t trigger_time_2 = static_cast<uint16_t>((byte_stream[j + 2] << 8) | byte_stream[j + 3]);
    packet.trigger_time =
        (trigger_time_1 << timing_shift_) | (trigger_time_2 & timing_mask_);
    j += 4;

    packet.logical_position =
        ((byte_stream[j] & abs_wind_mask_) << (8 - evt_wind_shift_)) |
        ((byte_stream[j + 1] >> evt_wind_shift_) & evt_wind_mask_);
    packet.physical_position = byte_stream[j + 1] & abs_wind_mask_;
    j += 2;

    std::memcpy(packet.raw_samples, byte_stream + j, 64);
    packet.footer = constructed_packet_footer_;
    packets.push_back(packet);
}

bool PacketParser::check_marker(const uint8_t* byte_stream,
                                size_t index,
                                const uint8_t* marker,
                                size_t marker_len) {
    for (size_t j = 0; j < marker_len; ++j) {
        if (byte_stream[index + j] != marker[j]) {
            return false;
        }
    }
    return true;
}

void PacketParser::process_byte_stream_segment_with_checks(std::vector<Packet>& packets,
                                                           const uint8_t* byte_stream,
                                                           size_t& i,
                                                           uint8_t& error_code,
                                                           size_t start_marker_len,
                                                           size_t stop_marker_len) {
    const size_t end_marker_position = i + packet_size_ - stop_marker_len;
    if (check_marker(byte_stream, end_marker_position, stop_marker_.data(), stop_marker_len)) {
        const size_t start_marker_position = i;
        if (check_marker(byte_stream,
                         start_marker_position,
                         start_marker_.data(),
                         start_marker_len)) {
            process_packet(packets, byte_stream, start_marker_position, error_code);
            error_code = 0;
            i += packet_size_;
        } else {
            error_code |= 0b10;
            process_packet(packets, byte_stream, i, error_code);
            i += packet_size_;
            spdlog::warn("Start marker not found at expected position");
        }
    } else {
        error_code |= 0b01;
        ++i;
    }
}

void PacketParser::process_byte_stream_segment_without_checks(std::vector<Packet>& packets,
                                                              const uint8_t* byte_stream,
                                                              size_t& i,
                                                              uint8_t&,
                                                              size_t,
                                                              size_t) {
    process_packet(packets, byte_stream, i, 0);
    i += packet_size_;
}

void PacketParser::process_leftovers(std::vector<Packet>& data_list,
                                     const uint8_t* byte_stream,
                                     size_t byte_stream_len,
                                     size_t leftovers_size,
                                     size_t packet_size,
                                     size_t start_marker_len,
                                     size_t stop_marker_len) {
    if (leftovers_size >= packet_size) {
        spdlog::warn("Leftovers size ({}) >= packet size ({})", leftovers_size, packet_size);
        return;
    }

    const size_t remaining_bytes = packet_size - leftovers_size;
    leftovers_.resize(packet_size);
    std::memcpy(leftovers_.data() + leftovers_size, byte_stream, remaining_bytes);

    const uint8_t* combined_ptr = leftovers_.data();
    if (!check_marker(combined_ptr, 0, start_marker_.data(), start_marker_len)) {
        spdlog::warn("Combined data does not start with the start marker");
    }

    const size_t end_marker_position = packet_size - stop_marker_len;
    if (end_marker_position <= byte_stream_len &&
        check_marker(combined_ptr, end_marker_position, stop_marker_.data(), stop_marker_len)) {
        process_packet(data_list, combined_ptr, 0, 0);
    }
}

std::vector<uint8_t> PacketParser::hexStringToBytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        const std::string byte_string = hex.substr(i, 2);
        bytes.push_back(static_cast<uint8_t>(std::stoi(byte_string, nullptr, 16)));
    }
    return bytes;
}

}  // namespace nalu_event_collector
