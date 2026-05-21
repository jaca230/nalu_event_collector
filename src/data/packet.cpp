/**
 * @file packet.cpp
 * @brief Implements the parsed packet data model.
 */

#include "nalu_event_collector/data/packet.h"

#include <cstring>
#include <iostream>

namespace nalu_event_collector {

Packet::Packet() = default;

Packet::Packet(uint16_t hdr,
               uint8_t ch,
               uint32_t trig_time,
               uint16_t log_pos,
               uint16_t phys_pos,
               uint8_t samples[64],
               uint16_t ftr,
               uint8_t full_info,
               uint16_t index)
    : header(hdr),
      info(full_info),
      channel(ch),
      trigger_time(trig_time),
      logical_position(log_pos),
      physical_position(phys_pos),
      parser_index(index),
      footer(ftr) {
    std::memcpy(raw_samples, samples, sizeof(raw_samples));
}

uint8_t Packet::get_error_code() const { return info & 0x0F; }

uint16_t Packet::get_size() const {
    return sizeof(header) + sizeof(channel) + sizeof(trigger_time) +
           sizeof(logical_position) + sizeof(physical_position) +
           sizeof(raw_samples) + sizeof(footer) + sizeof(info) +
           sizeof(parser_index);
}

void Packet::printout() const {
    std::cout << "Packet Details:\n";
    std::cout << "Header: 0x" << std::hex << header << std::dec << '\n';
    std::cout << "Info Byte: 0x" << std::hex << static_cast<int>(info) << std::dec << '\n';
    std::cout << "Channel: " << static_cast<int>(channel) << '\n';
    std::cout << "Trigger Time: " << trigger_time << '\n';
    std::cout << "Logical Position: " << logical_position << '\n';
    std::cout << "Physical Position: " << physical_position << '\n';
    std::cout << "Parser Index: " << parser_index << '\n';
    std::cout << "Footer: 0x" << std::hex << footer << std::dec << '\n';
    std::cout << "Error Code: 0x" << std::hex << static_cast<int>(get_error_code()) << std::dec << '\n';
}

}  // namespace nalu_event_collector
