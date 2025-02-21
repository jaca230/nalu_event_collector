#ifndef NALU_EVENT_WRAPPER_H
#define NALU_EVENT_WRAPPER_H

#include "nalu_event.h"  // Include the NaluEvent struct
#include <vector>        // For dynamic packet storage

class NaluEventWrapper {
public:
    // Default constructor
    NaluEventWrapper();

    // Constructor: Initializes the wrapper with a max number of packets
    NaluEventWrapper(uint16_t hdr, uint8_t extra_info, uint32_t idx, uint32_t ref_time, 
                     uint8_t size, uint16_t num, uint16_t ftr, size_t max_packets);

    // Method to add a packet to the event
    void add_packet(const NaluPacket& packet);

    // Serialize the event to the provided buffer (excluding unnecessary fields)
    void serialize_to_buffer(char* buffer) const;

    // Method to get the event size
    size_t get_size() const;

    // Getter for the NaluEvent
    NaluEvent& get_event();

    // Method to check if the event is complete
    bool is_event_complete(int windows, const std::vector<int>& channels) const;

private:
    NaluEvent event;  // NaluEvent instance
    size_t max_packets;  // Max number of packets allowed
};

#endif // NALU_EVENT_WRAPPER_H
