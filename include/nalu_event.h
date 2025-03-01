#ifndef NALU_EVENT_H
#define NALU_EVENT_H

#include <cstdint>
#include <vector>      // For std::vector
#include <chrono>      // For time tracking
#include "nalu_packet.h"
#include <memory>      // For std::unique_ptr

/**
 * @class NaluEvent
 * @brief Represents a single NALU event consisting of multiple NALU packets.
 *
 * The `NaluEvent` class stores the header, information, reference time, and a collection of NaluPackets
 * associated with a single event. It supports adding packets, serializing the event data, and checking
 * if the event is complete based on predefined windows and channels.
 */
class NaluEvent {
public:
    /** @brief Header of the event (2 bytes). */
    uint16_t header;

    /** @brief Extra information about the event (2 bytes). */
    uint16_t info;

    /** @brief Unique index for the event (4 bytes). */
    uint32_t index;

    /** @brief The reference time for the event (4 bytes). */
    uint32_t reference_time;

    /** @brief Packet size (2 bytes). */
    uint16_t packet_size;

    /** @brief The number of packets in the event (2 bytes). */
    uint16_t num_packets;

    /** @brief A dynamically allocated array of NaluPacket objects representing the packets in the event. */
    std::unique_ptr<NaluPacket[]> packets;

    /** @brief Footer of the event (2 bytes). */
    uint16_t footer;

    /** @brief Maximum number of packets that can be stored in this event. */
    size_t max_packets;

    /** @brief Timestamp when the event was created. */
    std::chrono::steady_clock::time_point creation_timestamp;

    /**
     * @brief Default constructor.
     *
     * Initializes all members to zero or null, and sets the creation timestamp to the current time.
     */
    NaluEvent();

    /**
     * @brief Constructor that initializes the event with specific values.
     * 
     * @param hdr The header of the event.
     * @param extra_info Extra information about the event.
     * @param idx The unique index of the event.
     * @param ref_time The reference time for the event.
     * @param size The size of each packet in the event.
     * @param num The number of packets in the event.
     * @param ftr The footer of the event.
     * @param max_num_packets The maximum number of packets that the event can store.
     */
    NaluEvent(uint16_t hdr, uint16_t extra_info, uint32_t idx, uint32_t ref_time, 
              uint16_t size, uint16_t num, uint16_t ftr, uint16_t max_num_packets);

    /**
     * @brief Gets the error code from the info field.
     *
     * The last 4 bits of the `info` field represent the error code.
     *
     * @return The error code (last 4 bits of `info`).
     */
    uint8_t get_error_code() const;

    /**
     * @brief Calculates the size of the entire event in bytes.
     * 
     * @return The size of the event (header, data, and all packets).
     */
    size_t get_size() const;

    /**
     * @brief Serializes the event into a buffer.
     *
     * The event is serialized into the provided buffer, which includes the header, packets, and footer.
     *
     * @param buffer The buffer to serialize the event into.
     */
    void serialize_to_buffer(char* buffer) const;

    /**
     * @brief Adds a packet to the event.
     *
     * If there is space for more packets (based on `max_packets`), the packet is added to the event.
     * If the event has reached the maximum packet count, an exception is thrown.
     *
     * @param packet The packet to add to the event.
     * @throws std::overflow_error if the number of packets exceeds the maximum allowed.
     */
    void add_packet(const NaluPacket& packet);

    /**
     * @brief Checks if the event is complete based on the number of packets and available channels.
     * 
     * The event is considered complete if the number of packets is greater than or equal to
     * the number of windows times the number of channels.
     *
     * @param windows The number of windows to check.
     * @param channels A list of channel identifiers.
     * @return True if the event is complete, false otherwise.
     */
    bool is_event_complete(int windows, const std::vector<int>& channels) const;

    /**
     * @brief Gets the creation timestamp of the event.
     * 
     * @return The time point when the event was created.
     */
    std::chrono::steady_clock::time_point get_creation_timestamp() const;

private:
    // Prevent copying and assignment
    NaluEvent(const NaluEvent&) = delete;
    NaluEvent& operator=(const NaluEvent&) = delete;
};

#endif // NALU_EVENT_H
