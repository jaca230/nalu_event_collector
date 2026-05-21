/**
 * @file udp_data_buffer.cpp
 * @brief Implements the raw UDP byte buffer used by the receiver.
 */

#include "nalu_event_collector/network/udp_data_buffer.h"

#include <spdlog/spdlog.h>

namespace nalu_event_collector {

UdpDataBuffer::UdpDataBuffer(size_t size) : capacity_(size) {}

void UdpDataBuffer::append(const uint8_t* data, size_t size) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (data == nullptr) {
        throw std::invalid_argument("Null pointer passed to append");
    }

    if (buffer_.size() + size > capacity_) {
        if (overflow_callback_) {
            overflow_callback_();
        }
        spdlog::error("UDP buffer overflow: append={} capacity={} current={}",
                      size,
                      capacity_,
                      buffer_.size());
        throw std::overflow_error("Buffer overflow");
    }

    buffer_.insert(buffer_.end(), data, data + size);
    cv_.notify_all();
}

bool UdpDataBuffer::pop(uint8_t& byte) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (buffer_.empty()) {
        return false;
    }

    byte = buffer_.front();
    buffer_.pop_front();
    return true;
}

size_t UdpDataBuffer::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return buffer_.size();
}

std::vector<uint8_t> UdpDataBuffer::getAllBytes() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<uint8_t> result(buffer_.begin(), buffer_.end());
    buffer_.clear();
    return result;
}

void UdpDataBuffer::waitForBytes(size_t min_count) {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this, min_count] { return buffer_.size() >= min_count; });
}

void UdpDataBuffer::setOverflowCallback(std::function<void()> callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    overflow_callback_ = std::move(callback);
}

bool UdpDataBuffer::isEmpty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return buffer_.empty();
}

bool UdpDataBuffer::isFull() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return buffer_.size() == capacity_;
}

}  // namespace nalu_event_collector
