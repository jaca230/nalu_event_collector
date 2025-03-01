#ifndef NALU_PACKET_H
#define NALU_PACKET_H

#include <cstdint>
#include <cstddef>  // For size_t

/// @class NaluPacket
/// @brief This class represents a single Nalu packet, including fields such as header, info byte,
/// channel, trigger time, logical/physical positions, raw samples, and footer.
/// The packet's structure is fixed at 80 bytes in total.
class NaluPacket {
public:
    /// @brief 2-byte header for the Nalu packet.
    uint16_t header;

    /// @brief 1-byte info field containing reserved bits and an error code.
    ///        The lower 4 bits store the error code.
    uint8_t info;

    /// @brief 1-byte channel identifier for the Nalu packet.
    uint8_t channel;

    /// @brief 4-byte trigger time representing the timestamp of the packet.
    uint32_t trigger_time;

    /// @brief 2-byte logical position of the Nalu packet.
    uint16_t logical_position;

    /// @brief 2-byte physical position of the Nalu packet.
    uint16_t physical_position;

    /// @brief 64-byte array storing raw samples in the Nalu packet.
    uint8_t raw_samples[64];

    /// @brief 2-byte index used to track the order of parsed packets.
    uint16_t parser_index;

    /// @brief 2-byte footer marking the end of the Nalu packet.
    uint16_t footer;

    /// @brief Default constructor initializes all member variables to zero.
    NaluPacket();

    /// @brief Constructor for initialization with full info byte and other Nalu packet fields.
    /// @param hdr The header value (2 bytes).
    /// @param ch The channel value (1 byte).
    /// @param trig_time The trigger time value (4 bytes).
    /// @param log_pos The logical position (2 bytes).
    /// @param phys_pos The physical position (2 bytes).
    /// @param samples The raw samples (64 bytes).
    /// @param ftr The footer value (2 bytes).
    /// @param full_info The full info byte (1 byte).
    /// @param index The parser index value (2 bytes, default value is 0).
    NaluPacket(uint16_t hdr, uint8_t ch, uint32_t trig_time, uint16_t log_pos, uint16_t phys_pos,
               uint8_t samples[64], uint16_t ftr, uint8_t full_info, uint16_t index = 0);

    /// @brief Extracts and returns the error code from the info byte (lower 4 bits).
    /// @return The error code (4 bits) from the info byte.
    uint8_t get_error_code() const;

    /// @brief Calculates and returns the size of the Nalu packet in bytes.
    /// @return The total size of the packet (should always be 80 bytes).
    uint16_t get_size() const;

private:
    // No private members, all fields are publicly accessible
};

#endif // NALU_PACKET_H
