#ifndef NALU_EVENT_H
#define NALU_EVENT_H

#include <chrono>  // For time tracking
#include <cstdint>
#include <memory>  // For std::unique_ptr
#include <vector>  // For std::vector

#include "nalu_packet.h"

/**
 * @class NaluEvent
 * @brief Represents a single NALU event consisting of multiple NALU packets.
 *
 * The `NaluEvent` class stores the header, information, reference time, and a
 * collection of NaluPackets associated with a single event. It supports adding
 * packets, serializing the event data, and checking if the event is complete
 * based on predefined windows and channels.
 */
class NaluEvent {
public:
    /// @brief Struct representing the header information of the event.
    #pragma pack(push, 1)
    struct Header {
        /** @brief Header of the event (2 bytes). */
        uint16_t header;

        /** @brief Extra information about the event (1 byte). The last 2 bits contain the trigger type */
        uint8_t info;

        /** @brief Unique index for the event (4 bytes). */
        uint32_t index;

        /** @brief The reference time for the event (4 bytes). */
        uint32_t reference_time;

        /** @brief The time threshold for the event (in ticks, 4 bytes). */
        uint32_t time_threshold;

        /** @brief Clock frequency used to convert ticks to duration (in Hz, 4 bytes). */
        uint32_t clock_frequency;

        /** @brief Packet size (2 bytes). */
        uint16_t packet_size;

        /** @brief Channel mask (8 bytes). */
        uint64_t channel_mask;

        /** @brief Number of digitized windows (1 byte). */
        uint8_t num_windows;

        /** @brief The number of packets in the event (2 bytes). */
        uint16_t num_packets;
    };
    #pragma pack(pop)

    /// @brief Footer of the event (2 bytes).
    #pragma pack(push, 1)
    struct Footer {
        /** @brief Footer of the event (2 bytes). */
        uint16_t footer;
    };
    #pragma pack(pop)

    /** @brief Header of the event. */
    Header header;

    /** @brief A dynamically allocated array of NaluPacket objects representing
     * the packets in the event. */
    std::unique_ptr<NaluPacket[]> packets;

    /** @brief Footer of the event. */
    Footer footer;

    /** @brief Maximum number of packets that can be stored in this event. */
    size_t max_packets;

    /** @brief Timestamp when the event was created. */
    std::chrono::steady_clock::time_point creation_timestamp;

    /**
     * @enum TriggerType
     * @brief Represents the trigger type encoded in bits 4 and 5 of the header.info field.
     *
     * Bits 4 and 5 of the info byte specify the trigger source for the event:
     * - Unknown (0)
     * - External (1)
     * - Internal (2)
     * - Immediate (3)
     */
    enum class TriggerType : uint8_t {
        Unknown = 0,
        External = 1,
        Internal = 2,
        Immediate = 3
    };

    /** Default constructor */
    NaluEvent();

    /**
     * Constructor that initializes the event with specific values.
     *
     * @param hdr The header of the event.
     * @param extra_info Extra information about the event.
     * @param idx The unique index of the event.
     * @param ref_time The reference time for the event.
     * @param time_thresh The time threshold for the event (in ticks).
     * @param clk_freq The clock frequency in Hz.
     * @param size The size of each packet in the event.
     * @param num The number of packets in the event.
     * @param ftr The footer of the event.
     * @param max_num_packets The maximum number of packets that the event can store.
     * @param channel_mask_value Channel mask value.
     * @param num_windows_value Number of digitized windows.
     */
    NaluEvent(uint16_t hdr, uint8_t extra_info, uint32_t idx,
              uint32_t ref_time, uint32_t time_thresh, uint32_t clk_freq,
              uint16_t size, uint16_t num,
              uint16_t ftr, uint16_t max_num_packets,
              uint64_t channel_mask_value, uint8_t num_windows_value);

    /** Prints out the event information (header and footer). */
    void print_event_info() const;

    /** Gets the error code from the info field (last 4 bits). */
    uint8_t get_error_code() const;

    /** Retrieves the trigger type encoded in bits 4 and 5 of the header.info field. */
    TriggerType get_trigger_type() const;

    /** Calculates the size of the entire event in bytes. */
    size_t get_size() const;

    /** Serializes the event into a buffer. */
    void serialize_to_buffer(char* buffer) const;

    /** Adds a packet to the event (throws if max packets exceeded). */
    void add_packet(const NaluPacket& packet);

    /**
     * Checks if the event is complete using embedded info.
     *
     * @return True if event is complete, false otherwise.
     */
    bool is_event_complete() const;

    /**
     * Checks if the event is complete based on windows, channels, trigger type string,
     * and external max time duration.
     */
    bool is_event_complete(int windows,
                           const std::vector<int>& channels,
                           std::string trigger_type_str,
                           std::chrono::steady_clock::duration max_time_between_events) const;

    /** Gets the creation timestamp of the event. */
    std::chrono::steady_clock::time_point get_creation_timestamp() const;

private:
    /** Helper function to count the number of active channels in the channel mask */
    int count_active_channels(uint64_t channel_mask) const;

    // Prevent copying and assignment
    NaluEvent(const NaluEvent&) = delete;
    NaluEvent& operator=(const NaluEvent&) = delete;
};

#endif  // NALU_EVENT_H
