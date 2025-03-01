#ifndef NALU_UDP_DATA_BUFFER_H
#define NALU_UDP_DATA_BUFFER_H

#include <deque>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <stdexcept>

/**
 * @class NaluUdpDataBuffer
 * @brief A thread-safe buffer for storing raw UDP packet data.
 *
 * This class provides methods to append data to the buffer, pop individual bytes,
 * retrieve all bytes, and wait for the buffer to contain a minimum number of bytes.
 * It also supports overflow notifications and checks if the buffer is empty or full.
 */
class NaluUdpDataBuffer {
public:
    /**
     * @brief Constructor: Initialize the buffer with a fixed size.
     * 
     * @param size The maximum size of the buffer in bytes.
     */
    NaluUdpDataBuffer(size_t size);

    /**
     * @brief Append raw bytes to the buffer.
     * 
     * This method inserts the provided data into the buffer. If appending the data 
     * would exceed the buffer's capacity, an overflow error is thrown, and an overflow
     * callback (if set) is invoked.
     * 
     * @param data Pointer to the data to append.
     * @param size The size of the data in bytes.
     * @throws std::overflow_error If the buffer overflows.
     */
    void append(const uint8_t* data, size_t size);

    /**
     * @brief Pop a single byte from the buffer.
     * 
     * This method removes and returns a single byte from the front of the buffer.
     * It returns false if the buffer is empty.
     * 
     * @param byte Reference to a byte variable to store the popped byte.
     * @return True if a byte is successfully popped, false if the buffer is empty.
     */
    bool pop(uint8_t& byte);

    /**
     * @brief Get the size of the buffer (number of bytes currently stored).
     * 
     * @return The number of bytes currently in the buffer.
     */
    size_t size() const;

    /**
     * @brief Retrieve all bytes currently in the buffer.
     * 
     * This method copies all the bytes from the buffer and clears the buffer afterward.
     * 
     * @return A vector containing all the bytes in the buffer.
     */
    std::vector<uint8_t> getAllBytes();

    /**
     * @brief Wait until at least a specified number of bytes are available in the buffer.
     * 
     * This method blocks the calling thread until the buffer contains at least 
     * 'min_count' bytes.
     * 
     * @param min_count The minimum number of bytes to wait for.
     */
    void waitForBytes(size_t min_count);

    /**
     * @brief Set a callback function for buffer overflow.
     * 
     * This method sets a callback function that will be called if the buffer overflows
     * (i.e., when attempting to append more data than the buffer can hold).
     * 
     * @param callback The callback function to invoke on overflow.
     */
    void setOverflowCallback(std::function<void()> callback);

    /**
     * @brief Check if the buffer is empty.
     * 
     * @return True if the buffer is empty, false otherwise.
     */
    bool isEmpty() const;

    /**
     * @brief Check if the buffer is full.
     * 
     * @return True if the buffer is full, false otherwise.
     */
    bool isFull() const;

private:
    std::deque<uint8_t> buffer_;   ///< A deque to store raw bytes.
    size_t capacity_;              ///< The maximum capacity of the buffer (in bytes).
    std::mutex mutex_;             ///< Mutex for thread safety.
    std::condition_variable cv_;   ///< Condition variable for waiting when needed.

    std::function<void()> overflow_callback_;  ///< Callback function to be called on buffer overflow.
};

#endif // NALU_UDP_DATA_BUFFER_H
