#include "nalu_udp_data_buffer.h"

#include <cstring>
#include <iostream>

#include "nalu_event_collector_logger.h"

NaluUdpDataBuffer::NaluUdpDataBuffer(size_t size) : capacity_(size) {}

void NaluUdpDataBuffer::append(const uint8_t* data, size_t size) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (data == nullptr) {
        NaluEventCollectorLogger::error("Null pointer passed to append method.");
        throw std::invalid_argument("Null pointer passed to append method.");
    }

    if (buffer_.size() + size > capacity_) {
        if (overflow_callback_) {
            overflow_callback_();
        }
        NaluEventCollectorLogger::error(
            "Buffer overflow: attempting to append " + std::to_string(size) +
            " bytes to a buffer of capacity " + std::to_string(capacity_) +
            " with " + std::to_string(buffer_.size()) +
            " bytes already stored.");
        throw std::overflow_error("Buffer overflow");
    }

    // Insert data into the buffer all at once
    buffer_.insert(buffer_.end(), data, data + size);
}


bool NaluUdpDataBuffer::pop(uint8_t& byte) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (buffer_.empty()) {
        return false;  // Buffer is empty
    }

    byte = buffer_.front();
    buffer_.pop_front();

    return true;
}

std::vector<uint8_t> NaluUdpDataBuffer::getAllBytes() {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<uint8_t> result(buffer_.begin(), buffer_.end());
    buffer_.clear();  // Clear the buffer after retrieving all bytes
    return result;
}

size_t NaluUdpDataBuffer::size() const { return buffer_.size(); }

void NaluUdpDataBuffer::waitForBytes(size_t min_count) {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this, min_count] { return buffer_.size() >= min_count; });
}

void NaluUdpDataBuffer::setOverflowCallback(std::function<void()> callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    overflow_callback_ = callback;
}

bool NaluUdpDataBuffer::isEmpty() const { return buffer_.empty(); }

bool NaluUdpDataBuffer::isFull() const { return buffer_.size() == capacity_; }
