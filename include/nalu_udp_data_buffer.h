#ifndef NALU_UDP_DATA_BUFFER_H
#define NALU_UDP_DATA_BUFFER_H

#include <deque>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <stdexcept>

class NaluUdpDataBuffer {
public:
    // Constructor: initialize buffer with fixed size
    NaluUdpDataBuffer(size_t size);

    // Append raw bytes to the buffer, throws error if full
    void append(const uint8_t* data, size_t size);

    // Pop a single byte from the buffer, returns false if empty
    bool pop(uint8_t& byte);

    // Get the size of the buffer (bytes currently in the buffer)
    size_t size() const;

    // Get all the bytes currently in the buffer
    std::vector<uint8_t> getAllBytes();

    // Wait until at least 'min_count' bytes are available
    void waitForBytes(size_t min_count);

    // Set the overflow callback for notifying the main thread
    void setOverflowCallback(std::function<void()> callback);

    // Check if the buffer is empty
    bool isEmpty() const;

    // Check if the buffer is full
    bool isFull() const;

private:
    std::deque<uint8_t> buffer_;   // Using deque to store raw bytes
    size_t capacity_;              // Maximum capacity of the buffer
    std::mutex mutex_;             // Mutex for thread safety
    std::condition_variable cv_;   // Condition variable for waiting

    std::function<void()> overflow_callback_;  // Callback for overflow notification
};

#endif // NALU_UDP_DATA_BUFFER_H
