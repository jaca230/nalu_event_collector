#ifndef NALU_UDP_DATA_BUFFER_H
#define NALU_UDP_DATA_BUFFER_H

#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <vector>

/**
 * @struct UdpPacket
 * @brief Represents a UDP packet with a 16-bit index and its payload.
 */
struct UdpPacket {
    uint16_t index;                ///< 16-bit index (wraps naturally on overflow)
    std::vector<uint8_t> data;     ///< Raw payload data
};

/**
 * @class NaluUdpDataBuffer
 * @brief Thread-safe container for indexed UDP packets.
 */
class NaluUdpDataBuffer {
   public:
    explicit NaluUdpDataBuffer(size_t capacity);

    void append(const uint8_t* data, size_t size);
    bool popPacket(UdpPacket& packet);
    std::vector<UdpPacket> getAllPackets();

    void waitForPackets(size_t min_count);
    void setOverflowCallback(std::function<void()> callback);

    size_t size() const;
    size_t sizeInBytes() const;
    bool isEmpty() const;
    bool isFull() const;

   private:
    std::deque<UdpPacket> packet_buffer_;
    size_t capacity_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::function<void()> overflow_callback_;
    uint16_t next_index_ = 0;  ///< Wraps naturally from 65535 to 0
};

#endif  // NALU_UDP_DATA_BUFFER_H
