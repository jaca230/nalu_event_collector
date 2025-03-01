#ifndef NALU_EVENT_BUILDER_H
#define NALU_EVENT_BUILDER_H

#include <vector>

#include "nalu_event_buffer.h"
#include "nalu_event_collector_params.h"
#include "nalu_packet.h"
#include "nalu_time_difference_calculator.h"

/**
 * @brief Class for building events from Nalu packets.
 *
 * The `NaluEventBuilder` class is responsible for collecting events from Nalu
 * packets. It manages the event buffer, handles the safety buffer zone, and
 * provides methods for collecting events and interacting with the internal
 * event buffer.
 */
class NaluEventBuilder {
   public:
    /**
     * @brief Constructor to initialize NaluEventBuilder with specific
     * parameters.
     *
     * @param channels The channels used for event collection.
     * @param windows The number of windows to be used for each event.
     * @param time_threshold The time threshold for triggering an event
     * (default: 5000).
     * @param max_trigger_time The maximum trigger time for events (default:
     * 16777216).
     * @param max_lookback The maximum lookback for events (default: 2).
     * @param event_max_size The maximum size of an event buffer (default:
     * 1024).
     * @param event_header The header to be added to the event (default:
     * 0xBBBB).
     * @param event_trailer The trailer to be added to the event (default:
     * 0xEEEE).
     */
    NaluEventBuilder(std::vector<int> channels, int windows,
                     int time_threshold = 5000,
                     uint32_t max_trigger_time = 16777216,
                     size_t max_lookback = 2, size_t event_max_size = 1024,
                     uint16_t event_header = 0xBBBB,
                     uint16_t event_trailer = 0xEEEE);

    /**
     * @brief Constructor that initializes NaluEventBuilder with parameters from
     * NaluEventCollectorParams.
     *
     * @param params The parameters for the NaluEventBuilder taken from
     * NaluEventCollectorParams.
     */
    NaluEventBuilder(const NaluEventCollectorParams& params);

    /**
     * @brief Collects events from the provided packets.
     *
     * This method processes the given packets and adds them to the event
     * buffer, managing the safety buffer and event index.
     *
     * @param packets The list of Nalu packets to be processed.
     */
    void collect_events(const std::vector<NaluPacket>& packets);

    /**
     * @brief Gets the internal event buffer.
     *
     * This method provides access to the internal event buffer for event
     * management.
     *
     * @return Reference to the event buffer.
     */
    NaluEventBuffer& get_event_buffer() { return event_buffer; }

    /**
     * @brief Gets the number of windows used for event collection.
     *
     * @return The number of windows.
     */
    int get_windows() const { return windows; }

    /**
     * @brief Gets the channels used for event collection.
     *
     * @return The list of channel numbers.
     */
    const std::vector<int>& get_channels() const { return channels; }

    /**
     * @brief Sets the maximum size for the post-event safety buffer counter.
     *
     * This value determines when the safety buffer zone should be exited.
     *
     * @param counter_max The new maximum size for the safety buffer counter.
     */
    void set_post_event_safety_buffer_counter_max(size_t counter_max);

   private:
    std::vector<int> channels;  ///< The channels for event collection
    int windows;                ///< The number of windows for event collection

    size_t post_event_safety_buffer_counter_max;  ///< The maximum safety buffer
                                                  ///< size
    size_t post_event_safety_buffer_counter =
        0;  ///< The current safety buffer counter
    bool in_safety_buffer_zone =
        false;  ///< Flag indicating whether we're in the safety buffer zone

    uint32_t event_index = 0;  ///< The event index tracker

    NaluEventBuffer
        event_buffer;  ///< The internal event buffer for managing events
    NaluTimeDifferenceCalculator
        time_diff_calculator;  ///< Time difference calculator

    /**
     * @brief Manages the safety buffer by adjusting the buffer counter and
     * determining when to exit the safety zone.
     */
    void manage_safety_buffer();
};

#endif  // NALU_EVENT_BUILDER_H
