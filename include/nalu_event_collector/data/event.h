/**
 * @file event.h
 * @brief Event data model built from one or more parsed packets.
 */

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "nalu_event_collector/data/packet.h"

namespace nalu_event_collector {

/**
 * @brief Represents one collected event and its serialized metadata.
 */
class Event {
  public:
#pragma pack(push, 1)
    /**
     * @brief Serialized event header written ahead of packet payloads.
     */
    struct Header {
        uint16_t header;
        uint8_t info;
        uint32_t index;
        uint32_t reference_time;
        uint32_t time_threshold;
        uint32_t event_completion_time_us;
        uint32_t clock_frequency;
        uint16_t packet_size;
        uint64_t channel_mask;
        uint8_t num_windows;
        uint16_t num_packets;
    };
#pragma pack(pop)

#pragma pack(push, 1)
    /**
     * @brief Serialized event trailer written after packet payloads.
     */
    struct Footer {
        uint16_t footer;
    };
#pragma pack(pop)

    /**
     * @brief Trigger source encoded in the header info field.
     */
    enum class TriggerType : uint8_t {
        Unknown = 0,
        External = 1,
        Internal = 2,
        Immediate = 3,
    };

    /** @brief Event header metadata. */
    Header header;

    /** @brief Owned packet storage for this event. */
    std::unique_ptr<Packet[]> packets;

    /** @brief Event trailer metadata. */
    Footer footer;

    /** @brief Maximum number of packets allowed in this event. */
    size_t max_packets;

    /** @brief Creation timestamp used for timeout-based completion logic. */
    std::chrono::steady_clock::time_point creation_timestamp;

    /** @brief Construct an empty event shell. */
    Event();

    /** @brief Construct a fully parameterized event container. */
    Event(uint16_t hdr,
          uint8_t extra_info,
          uint32_t idx,
          uint32_t ref_time,
          uint32_t time_thresh,
          uint32_t clk_freq,
          uint32_t event_completion_time_us,
          uint16_t size,
          uint16_t num,
          uint16_t ftr,
          uint16_t max_num_packets,
          uint64_t channel_mask_value,
          uint8_t num_windows_value);

    /** @brief Print a readable event summary to stdout. */
    void print_event_info() const;

    /** @brief Return the low-level packet error code encoded in the info byte. */
    uint8_t get_error_code() const;

    /** @brief Decode the trigger source from the info byte. */
    TriggerType get_trigger_type() const;

    /** @brief Return the serialized size of the event in bytes. */
    size_t get_size() const;

    /** @brief Serialize the event header, packets, and footer into @p buffer. */
    void serialize_to_buffer(char* buffer) const;

    /** @brief Append one packet to the event. */
    void add_packet(const Packet& packet);

    /** @brief Determine completeness using embedded event metadata. */
    bool is_event_complete() const;

    /** @brief Determine completeness using explicit compatibility arguments. */
    bool is_event_complete(int windows,
                           const std::vector<int>& channels,
                           std::string trigger_type_str,
                           std::chrono::steady_clock::duration max_time_between_events) const;

    /** @brief Return the event creation timestamp. */
    std::chrono::steady_clock::time_point get_creation_timestamp() const;

  private:
    int count_active_channels(uint64_t channel_mask) const;

    Event(const Event&) = delete;
    Event& operator=(const Event&) = delete;
};

}  // namespace nalu_event_collector
