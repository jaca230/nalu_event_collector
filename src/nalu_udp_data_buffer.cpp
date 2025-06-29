#include "nalu_udp_data_buffer.h"
#include "nalu_event_collector_logger.h"

NaluUdpDataBuffer::NaluUdpDataBuffer(size_t capacity)
    : capacity_(capacity) {}

void NaluUdpDataBuffer::append(const uint8_t* data, size_t size) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!data) {
        NaluEventCollectorLogger::error("Null data passed to append.");
        throw std::invalid_argument("Null data pointer");
    }

    if (packet_buffer_.size() >= capacity_) {
        if (overflow_callback_) overflow_callback_();
        NaluEventCollectorLogger::error("Buffer overflow (packet count)");
        throw std::overflow_error("Buffer overflow");
    }

    UdpPacket packet;
    packet.index = next_index_++;
    packet.data.assign(data, data + size);
    packet_buffer_.push_back(std::move(packet));

    cv_.notify_all();
}

bool NaluUdpDataBuffer::popPacket(UdpPacket& packet) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (packet_buffer_.empty()) return false;

    packet = std::move(packet_buffer_.front());
    packet_buffer_.pop_front();
    return true;
}

std::vector<UdpPacket> NaluUdpDataBuffer::getAllPackets() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<UdpPacket> result(packet_buffer_.begin(), packet_buffer_.end());
    packet_buffer_.clear();
    return result;
}

void NaluUdpDataBuffer::waitForPackets(size_t min_count) {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this, min_count] { return packet_buffer_.size() >= min_count; });
}

void NaluUdpDataBuffer::setOverflowCallback(std::function<void()> callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    overflow_callback_ = std::move(callback);
}

size_t NaluUdpDataBuffer::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return packet_buffer_.size();
}

size_t NaluUdpDataBuffer::sizeInBytes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t total_bytes = 0;
    for (const auto& packet : packet_buffer_) {
        total_bytes += packet.data.size();
    }
    return total_bytes;
}

bool NaluUdpDataBuffer::isEmpty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return packet_buffer_.empty();
}

bool NaluUdpDataBuffer::isFull() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return packet_buffer_.size() >= capacity_;
}
