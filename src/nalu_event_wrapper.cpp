#include "nalu_event_wrapper.h"
#include <stdexcept>  // For exceptions
#include <cstring>    // For std::memcpy

// Default constructor
// WARNING: This will NOT preallocate memory for the packets
NaluEventWrapper::NaluEventWrapper() : event(), max_packets(0) {}

// Constructor for initializing the wrapper with a max number of packets
NaluEventWrapper::NaluEventWrapper(uint16_t hdr, uint8_t extra_info, uint32_t idx, uint32_t ref_time, 
                                   uint8_t size, uint16_t num, uint16_t ftr, size_t max_packets)
    : event(hdr, extra_info, idx, ref_time, size, num, ftr, max_packets), max_packets(max_packets) {}

// Method to add a packet to the event
void NaluEventWrapper::add_packet(const NaluPacket& packet) {
    if (event.num_packets < max_packets) {
        event.packets[event.num_packets] = packet;
        event.num_packets++;
    } else {
        throw std::overflow_error("Maximum number of packets exceeded.");
    }
}

void NaluEventWrapper::serialize_to_buffer(char* buffer) const {
    if (!buffer) return;

    size_t offset = 0;

    // Copy everything up to `packets`
    std::memcpy(buffer + offset, &event, offsetof(NaluEvent, packets));
    offset += offsetof(NaluEvent, packets);

    // Copy packet data
    std::memcpy(buffer + offset, event.packets, event.num_packets * sizeof(NaluPacket));
    offset += event.num_packets * sizeof(NaluPacket);

    // Copy footer
    std::memcpy(buffer + offset, &event.footer, sizeof(event.footer));
}

// Method to get the event size
size_t NaluEventWrapper::get_size() const {
    return event.get_size();
}

// Getter for the NaluEvent
NaluEvent& NaluEventWrapper::get_event() {
    return event;
}

// Method to check if the event is complete based on some logical criteria
bool NaluEventWrapper::is_event_complete(int windows, const std::vector<int>& channels) const {
    return event.num_packets >= windows * channels.size();
}
