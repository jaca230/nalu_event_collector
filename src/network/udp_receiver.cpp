/**
 * @file udp_receiver.cpp
 * @brief Implements socket setup and UDP receive-loop handling.
 */

#include "nalu_event_collector/network/udp_receiver.h"

#include <arpa/inet.h>
#include <unistd.h>

#include <cstring>
#include <stdexcept>

#include <spdlog/spdlog.h>

namespace nalu_event_collector {

UdpReceiver::UdpReceiver(const std::string& address,
                         uint16_t port,
                         size_t buffer_size,
                         size_t max_packet_size,
                         int timeout_sec)
    : address_(address),
      port_(port),
      socket_fd_(-1),
      running_(false),
      data_buffer_(buffer_size),
      max_packet_size_(max_packet_size),
      timeout_sec_(timeout_sec) {}

UdpReceiver::UdpReceiver(const UdpReceiverConfig& config)
    : UdpReceiver(config.address,
                  config.port,
                  config.buffer_size,
                  config.max_packet_size,
                  config.timeout_sec) {}

UdpReceiver::~UdpReceiver() { stop(); }

void UdpReceiver::initSocket() {
    socket_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd_ < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_);
    inet_pton(AF_INET, address_.c_str(), &server_addr.sin_addr);

    if (bind(socket_fd_, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
        throw std::runtime_error("Failed to bind socket");
    }

    if (timeout_sec_ > 0) {
        timeval timeout{};
        timeout.tv_sec = timeout_sec_;
        timeout.tv_usec = 0;
        setsockopt(socket_fd_, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    }
}

void UdpReceiver::start() {
    if (running_) {
        return;
    }

    running_ = true;
    initSocket();
    receiver_thread_ = std::thread(&UdpReceiver::receiveLoop, this);
}

void UdpReceiver::stop() {
    if (!running_) {
        return;
    }

    running_ = false;

    if (socket_fd_ >= 0) {
        close(socket_fd_);
        socket_fd_ = -1;
    }

    if (receiver_thread_.joinable()) {
        receiver_thread_.join();
    }
}

void UdpReceiver::receiveLoop() {
    try {
        auto udp_packet_buffer = std::make_unique<uint8_t[]>(max_packet_size_);

        while (running_) {
            sockaddr_in client_addr{};
            socklen_t client_addr_len = sizeof(client_addr);
            const ssize_t received_bytes =
                recvfrom(socket_fd_,
                         udp_packet_buffer.get(),
                         max_packet_size_,
                         0,
                         reinterpret_cast<sockaddr*>(&client_addr),
                         &client_addr_len);

            if (received_bytes < 0) {
                if (!running_) {
                    break;
                }
                continue;
            }

            if (received_bytes < 16) {
                spdlog::warn("Malformed UDP packet: too small ({} bytes)", received_bytes);
                continue;
            }

            uint16_t payload_size = 0;
            std::memcpy(&payload_size, udp_packet_buffer.get(), sizeof(uint16_t));
            payload_size = ntohs(payload_size);

            if (payload_size != static_cast<uint16_t>(received_bytes - 16)) {
                spdlog::warn("Malformed UDP packet: expected payload size {}, received {}",
                             payload_size,
                             received_bytes - 16);
                continue;
            }

            data_buffer_.append(udp_packet_buffer.get() + 16, payload_size);
        }
    } catch (const std::exception& error) {
        spdlog::error("Receiver thread error: {}", error.what());
    }
}

UdpDataBuffer& UdpReceiver::getDataBuffer() { return data_buffer_; }

}  // namespace nalu_event_collector
