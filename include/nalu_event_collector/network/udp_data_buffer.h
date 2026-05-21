/**
 * @file udp_data_buffer.h
 * @brief Thread-safe raw byte buffer used between UDP reception and parsing.
 */

#pragma once

#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <vector>

namespace nalu_event_collector {

/**
 * @brief Bounded FIFO-style byte buffer for raw UDP payload data.
 */
class UdpDataBuffer {
  public:
    /** @brief Construct a buffer with a fixed byte capacity. */
    explicit UdpDataBuffer(size_t size);

    /** @brief Append a byte range to the buffer. */
    void append(const uint8_t* data, size_t size);

    /** @brief Pop one byte from the front of the buffer if available. */
    bool pop(uint8_t& byte);

    /** @brief Return the number of buffered bytes. */
    size_t size() const;

    /** @brief Return all currently buffered bytes and clear the buffer. */
    std::vector<uint8_t> getAllBytes();

    /** @brief Block until at least @p min_count bytes are available. */
    void waitForBytes(size_t min_count);

    /** @brief Set a callback invoked before throwing on overflow. */
    void setOverflowCallback(std::function<void()> callback);

    /** @brief Return true when the buffer is empty. */
    bool isEmpty() const;

    /** @brief Return true when the buffer is at full capacity. */
    bool isFull() const;

  private:
    std::deque<uint8_t> buffer_;
    size_t capacity_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::function<void()> overflow_callback_;
};

}  // namespace nalu_event_collector
