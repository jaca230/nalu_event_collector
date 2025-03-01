#ifndef NALU_EVENT_H
#define NALU_EVENT_H

#include <cstdint>
#include <vector>      // For std::vector
#include <chrono>      // For time tracking
#include "nalu_packet.h"
#include <memory>      // For std::unique_ptr

class NaluEvent {
public:
    uint16_t header;           // 2 bytes
    uint16_t info;             // 2 byte
    uint32_t index;            // 4 bytes
    uint32_t reference_time;   // 4 bytes
    uint16_t packet_size;      // 2 byte
    uint16_t num_packets;      // 2 bytes
    std::unique_ptr<NaluPacket[]> packets;  // Pointer to dynamic array of NaluPacket (size depends on num_packets)
    uint16_t footer;           // 2 bytes
                               // SUM: 20 bytes + (80 bytes * num_packets)

    // Default constructor
    NaluEvent();

    // Constructor to initialize the class with specific values
    NaluEvent(uint16_t hdr, uint16_t extra_info, uint32_t idx, uint32_t ref_time, 
              uint16_t size, uint16_t num, uint16_t ftr, uint16_t max_num_packets);

    // Destructor automatically handled by unique_ptr

    // Deleted copy constructor and assignment operator
    NaluEvent(const NaluEvent&) = delete;
    NaluEvent& operator=(const NaluEvent&) = delete;

    uint8_t get_error_code() const;
    size_t get_size() const;
    void serialize_to_buffer(char* buffer) const;
    void add_packet(const NaluPacket& packet);
    bool is_event_complete(int windows, const std::vector<int>& channels) const;
    std::chrono::steady_clock::time_point get_creation_timestamp() const;

private:
    size_t max_packets; 
    std::chrono::steady_clock::time_point creation_timestamp;  
};

#endif // NALU_EVENT_H
