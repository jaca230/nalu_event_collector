#ifndef NALU_EVENT_BUFFER_H
#define NALU_EVENT_BUFFER_H

#include <functional>
#include <memory>
#include <mutex>  // For thread safety
#include <vector>

#include "nalu_event.h"
#include "nalu_time_difference_calculator.h"

/**
 * @class NaluEventBuffer
 * @brief Class for managing a buffer of NaluEvents, including adding,
 * retrieving, and removing events.
 *
 * This class is responsible for holding and managing a collection of
 * `NaluEvent` objects. It includes methods to add new events, retrieve events,
 * and remove events based on various criteria. The class is thread-safe, with
 * synchronization ensured using a mutex.
 */
class NaluEventBuffer {
   public:
    /**
     * @brief Constructs a NaluEventBuffer with the specified parameters.
     * @param max_events The maximum number of events the buffer can hold.
     * @param time_diff_calculator The calculator used to compute time differences.
     * @param max_lookback The maximum number of past events to consider when adding a new packet.
     * @param channels A vector of channel identifiers.
     * @param windows The number of windows used for constructing events.
     * @param event_header The header used for constructing events.
     * @param event_trailer The trailer used for constructing events.
     * @param trigger_type The type of trigger to use ("self", "ext", etc.).
     */
    NaluEventBuffer(size_t max_events,
        NaluTimeDifferenceCalculator& time_diff_calculator,
        size_t max_lookback,
        const std::vector<int>& channels,
        uint8_t windows,
        uint16_t event_header,
        uint16_t event_trailer,
        std::string trigger_type);

    /** @brief Destructor */
    ~NaluEventBuffer();

    /**
     * @brief Adds a new event to the buffer.
     * @param event A unique pointer to the event to be added.
     *
     * Throws an exception if the buffer has reached its maximum capacity.
     */
    void add_event(std::unique_ptr<NaluEvent> event);

    /**
     * @brief Retrieves all events currently in the buffer.
     * @return A reference to a vector of unique pointers to events.
     */
    std::vector<std::unique_ptr<NaluEvent>>& get_events();

    /**
     * @brief Retrieves the latest event in the buffer.
     * @return A reference to the latest event.
     *
     * Throws an exception if the buffer is empty.
     */
    NaluEvent& get_latest_event();

    /**
     * @brief Retrieves an event by its index.
     * @param index The index of the event to retrieve.
     * @return A reference to the event at the specified index.
     *
     * Throws an exception if the index is out of range.
     */
    NaluEvent& get_event_by_index(size_t index);

    /**
     * @brief Sets the callback function to be invoked when the buffer
     * overflows.
     * @param callback A function to call when the buffer is full.
     */
    void set_on_overflow_callback(std::function<void()> callback);

    /**
     * @brief Sets the maximum number of events the buffer can hold.
     * @param max_events The new maximum number of events.
     */
    void set_max_events(size_t max_events);

    /**
     * @brief Retrieves events that were added after a specified timestamp.
     *
     * This method performs a binary search to find the first event added after
     * the provided timestamp.
     *
     * @param timestamp The timestamp to compare against.
     * @param seed_index The index from which to start searching (default is
     * -1).
     * @return A vector of pointers to events added after the timestamp.
     */
    std::vector<NaluEvent*> get_events_after_timestamp(
        const std::chrono::steady_clock::time_point& timestamp,
        ssize_t seed_index = -1) const;

    /**
     * @brief Removes events that were added before a specified timestamp.
     *
     * This method performs a binary search to find the first event added after
     * the provided timestamp and removes all events before it.
     *
     * @param timestamp The timestamp to compare against.
     * @param seed_index The index from which to start searching (default is
     * -1).
     * @return The number of events removed.
     */
    size_t remove_events_before_timestamp(
        const std::chrono::steady_clock::time_point& timestamp,
        ssize_t seed_index = -1);

    /**
     * @brief Retrieves events after a specified index, inclusive.
     * @param index The index from which to start retrieving events.
     * @return A vector of pointers to events after the given index.
     */
    std::vector<NaluEvent*> get_events_after_index_inclusive(
        size_t index) const;

    /**
     * @brief Removes events before a specified index (exclusive).
     * @param index The index to compare against.
     * @return The number of events removed.
     */
    size_t remove_events_before_index_exclusive(size_t index);

    /**
     * @brief Adds a packet to the event buffer, creating or updating events as
     * needed.
     * @param packet The packet to add to the buffer.
     * @param in_safety_buffer_zone A reference to a flag indicating if the
     * packet is within the safety buffer zone.
     * @param event_index A reference to the event index, incremented for each
     * new event.
     */
    void add_packet(const NaluPacket& packet, bool& in_safety_buffer_zone,
                    uint32_t& event_index);


    /**
     * @brief Clears all events in the buffer.
     *
     * This method removes all events from the buffer, effectively resetting it.
     */
    void clear();

   private:
    mutable std::mutex buffer_mutex;  ///< Mutex to protect the event buffer
    std::vector<std::unique_ptr<NaluEvent>>
        events;         ///< The list of events in the buffer
    size_t max_events;  ///< The maximum number of events the buffer can hold
    std::function<void()>
        overflow_callback;  ///< Callback function to handle buffer overflow

    // Member variables for configuration
    NaluTimeDifferenceCalculator&
        time_diff_calculator;  ///< The time difference calculator
    size_t max_lookback;     ///< The maximum number of past events to consider
    size_t max_event_size;   ///< The maximum size of a single event
    uint8_t windows;         ///< The number of windows used for constructing events
    uint64_t channel_mask;   ///< The mask of active channels
    uint16_t event_header;   ///< The header used to construct events
    uint16_t event_trailer;  ///< The trailer used to construct events
    uint8_t extra_info;  ///< Extra information about the event (e.g., trigger type)

    /**
     * @brief adds an event to the buffer (not threadsafe).
     *
     * This methods add a singular event to the buffer in a not threadsafe manner.
     * It's pupose is just to consolidate code.
     */
    void add_event_helper(std::unique_ptr<NaluEvent>& event);
};

#endif  // NALU_EVENT_BUFFER_H
