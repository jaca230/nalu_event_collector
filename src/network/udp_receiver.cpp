/**
 * @file udp_receiver.cpp
 * @brief Implements socket setup and UDP receive-loop handling.
 */

#include "nalu_event_collector/network/udp_receiver.h"

#include <arpa/inet.h>
#include <spdlog/spdlog.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <stdexcept>

namespace nalu_event_collector {

UdpReceiver::UdpReceiver(const std::string& address, uint16_t port,
                         size_t buffer_size, size_t max_packet_size,
                         int timeout_sec)
    : address_(address),
      port_(port),
      socket_fd_(-1),
      running_(false),
      data_buffer_(buffer_size),
      max_packet_size_(max_packet_size),
      timeout_sec_(timeout_sec) {}

UdpReceiver::UdpReceiver(const UdpReceiverConfig& config)
    : UdpReceiver(config.address, config.port, config.buffer_size,
                  config.max_packet_size, config.timeout_sec) {}

UdpReceiver::~UdpReceiver() { stop(); }

void UdpReceiver::initSocket() {
    socket_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd_ < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    int receive_bytes = 4 * 1024 * 1024;
    setsockopt(socket_fd_, SOL_SOCKET, SO_RCVBUF, &receive_bytes,
               sizeof(receive_bytes));
    int report_overflow = 1;
    setsockopt(socket_fd_, SOL_SOCKET, SO_RXQ_OVFL, &report_overflow,
               sizeof(report_overflow));
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_);
    inet_pton(AF_INET, address_.c_str(), &server_addr.sin_addr);

    if (bind(socket_fd_, reinterpret_cast<sockaddr*>(&server_addr),
             sizeof(server_addr)) < 0) {
        throw std::runtime_error("Failed to bind socket");
    }

    if (timeout_sec_ > 0) {
        timeval timeout{};
        timeout.tv_sec = timeout_sec_;
        timeout.tv_usec = 0;
        setsockopt(socket_fd_, SOL_SOCKET, SO_RCVTIMEO, &timeout,
                   sizeof(timeout));
    }
}

void UdpReceiver::start() {
    if (running_) {
        return;
    }

    initSocket();
    running_ = true;
    receiver_thread_ = std::thread(&UdpReceiver::receiveLoop, this);
}

void UdpReceiver::stop() {
    if (!running_) {
        return;
    }

    running_ = false;

    if (socket_fd_ >= 0) {
        // Closing a descriptor in one thread does not reliably interrupt a
        // blocking recvfrom() in another thread on Linux. Wake the receiver
        // explicitly so shutdown is not delayed by the configured socket
        // timeout.
        shutdown(socket_fd_, SHUT_RDWR);
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
            iovec iov{udp_packet_buffer.get(), max_packet_size_};
            alignas(cmsghdr) char control[CMSG_SPACE(sizeof(uint32_t))]{};
            msghdr message{};
            message.msg_name = &client_addr;
            message.msg_namelen = client_addr_len;
            message.msg_iov = &iov;
            message.msg_iovlen = 1;
            message.msg_control = control;
            message.msg_controllen = sizeof(control);
            const ssize_t received_bytes = recvmsg(socket_fd_, &message, 0);
            if (received_bytes >= 0 && (message.msg_flags & MSG_TRUNC))
              throw std::runtime_error(
                  "UDP datagram truncated: increase max_packet_size");
            for (auto *c = CMSG_FIRSTHDR(&message); received_bytes >= 0 && c;
                 c = CMSG_NXTHDR(&message, c)) {
              if (c->cmsg_level == SOL_SOCKET && c->cmsg_type == SO_RXQ_OVFL) {
                uint32_t dropped = 0;
                std::memcpy(&dropped, CMSG_DATA(c), sizeof(dropped));
                if (dropped)
                  throw std::runtime_error("Kernel UDP socket dropped " +
                                           std::to_string(dropped) +
                                           " datagrams");
              }
            }

            if (received_bytes <= 0 && !running_) {
                break;
            }
            if (received_bytes < 0) {
              if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
                throw std::runtime_error("UDP receive failed");
              continue;
            }

            if (received_bytes < 16) {
                spdlog::warn("Malformed UDP packet: too small ({} bytes)",
                             received_bytes);
                throw std::runtime_error("Malformed UDP transport header");
            }

            uint16_t payload_size = 0;
            std::memcpy(&payload_size, udp_packet_buffer.get(),
                        sizeof(uint16_t));
            payload_size = ntohs(payload_size);

            if (payload_size != static_cast<uint16_t>(received_bytes - 16)) {
                spdlog::warn(
                    "Malformed UDP packet: expected payload size {}, received "
                    "{}",
                    payload_size, received_bytes - 16);
                throw std::runtime_error("UDP transport length mismatch");
            }

            data_buffer_.append(udp_packet_buffer.get() + 16, payload_size);
        }
    } catch (const std::exception& error) {
      {
        std::lock_guard lock(error_mutex_);
        error_ = error.what();
      }
        spdlog::error("Receiver thread error: {}", error.what());
    }
}

void UdpReceiver::check_health() const {
  std::lock_guard lock(error_mutex_);
  if (!error_.empty())
    throw std::runtime_error(error_);
}

UdpDataBuffer& UdpReceiver::getDataBuffer() { return data_buffer_; }

}  // namespace nalu_event_collector
