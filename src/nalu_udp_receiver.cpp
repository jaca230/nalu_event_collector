#include "nalu_udp_receiver.h"

#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>

#include "nalu_event_collector_logger.h"

NaluUdpReceiver::NaluUdpReceiver(const std::string& address, uint16_t port,
                                 size_t buffer_size, size_t max_packet_size,
                                 int timeout_sec)
    : address_(address),
      port_(port),
      socket_fd_(-1),
      running_(false),
      nalu_udp_data_buffer_(buffer_size),
      max_packet_size_(max_packet_size),
      timeout_sec_(timeout_sec) {}

NaluUdpReceiver::NaluUdpReceiver(const NaluUdpReceiverParams& params)
    : NaluUdpReceiver(params.address, params.port, params.buffer_size,
                      params.max_packet_size, params.timeout_sec) {}

NaluUdpReceiver::~NaluUdpReceiver() {
    stop();
}

void NaluUdpReceiver::initSocket() {
    socket_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd_ < 0)
        throw std::runtime_error("Failed to create UDP socket");

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    inet_pton(AF_INET, address_.c_str(), &addr.sin_addr);

    if (bind(socket_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        throw std::runtime_error("Failed to bind UDP socket");

    if (timeout_sec_ > 0) {
        struct timeval timeout{};
        timeout.tv_sec = timeout_sec_;
        timeout.tv_usec = 0;
        setsockopt(socket_fd_, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    }
}

void NaluUdpReceiver::start() {
    if (running_) return;
    initSocket();
    running_ = true;
    receiver_thread_ = std::thread(&NaluUdpReceiver::receiveLoop, this);
}

void NaluUdpReceiver::stop() {
    if (!running_) return;
    running_ = false;

    if (socket_fd_ >= 0) {
        close(socket_fd_);
        socket_fd_ = -1;
    }

    if (receiver_thread_.joinable()) {
        receiver_thread_.join();
    }
}

void NaluUdpReceiver::receiveLoop() {
    try {
        uint8_t* buffer = new uint8_t[max_packet_size_];

        while (running_) {
            sockaddr_in client_addr{};
            socklen_t addr_len = sizeof(client_addr);
            ssize_t n = recvfrom(socket_fd_, buffer, max_packet_size_, 0,
                                 (struct sockaddr*)&client_addr, &addr_len);

            if (n < 0) continue;
            if (n < 16) continue;  // malformed

            uint16_t payload_size;
            std::memcpy(&payload_size, buffer, sizeof(uint16_t));
            payload_size = ntohs(payload_size);

            if (payload_size != static_cast<uint16_t>(n - 16)) continue;

            nalu_udp_data_buffer_.append(buffer + 16, payload_size);
        }

        delete[] buffer;
    } catch (const std::exception& e) {
        NaluEventCollectorLogger::error("Receiver thread error: " + std::string(e.what()));
    }
}

NaluUdpDataBuffer& NaluUdpReceiver::getDataBuffer() {
    return nalu_udp_data_buffer_;
}
