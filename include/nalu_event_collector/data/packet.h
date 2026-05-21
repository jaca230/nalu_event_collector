/**
 * @file packet.h
 * @brief Parsed packet data model used by the event collection pipeline.
 */

#pragma once

#include <cstddef>
#include <cstdint>

namespace nalu_event_collector {

/**
 * @brief Represents one parsed packet extracted from the UDP byte stream.
 */
class Packet {
  public:
    /** @brief Synthetic packet header used by the collector. */
    uint16_t header = 0;

    /** @brief Info byte containing parser status and error bits. */
    uint8_t info = 0;

    /** @brief Decoded channel identifier. */
    uint8_t channel = 0;

    /** @brief Decoded trigger timestamp in board ticks. */
    uint32_t trigger_time = 0;

    /** @brief Logical window position within the event. */
    uint16_t logical_position = 0;

    /** @brief Physical window position reported by the hardware. */
    uint16_t physical_position = 0;

    /** @brief Raw ADC sample payload. */
    uint8_t raw_samples[64] = {};

    /** @brief Monotonic parser-local packet index. */
    uint16_t parser_index = 0;

    /** @brief Synthetic packet footer used by the collector. */
    uint16_t footer = 0;

    /** @brief Construct a zero-initialized packet. */
    Packet();

    /** @brief Construct a packet with fully decoded field values. */
    Packet(uint16_t hdr,
           uint8_t ch,
           uint32_t trig_time,
           uint16_t log_pos,
           uint16_t phys_pos,
           uint8_t samples[64],
           uint16_t ftr,
           uint8_t full_info,
           uint16_t index = 0);

    /** @brief Return the parser error code stored in the info byte. */
    uint8_t get_error_code() const;

    /** @brief Return the serialized packet size in bytes. */
    uint16_t get_size() const;

    /** @brief Print a readable packet summary to stdout. */
    void printout() const;
};

}  // namespace nalu_event_collector
