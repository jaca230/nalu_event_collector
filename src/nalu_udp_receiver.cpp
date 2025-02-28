#include "nalu_udp_receiver.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

#include "nalu_udp_receiver.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

NaluUdpReceiver::NaluUdpReceiver(const std::string& address, uint16_t port, size_t buffer_size, size_t max_packet_size, int timeout_sec)
    : address_(address),
      port_(port),
      max_packet_size_(max_packet_size),
      nalu_udp_data_buffer_(buffer_size),
      socket_fd_(-1),
      running_(false),
      timeout_sec_(timeout_sec) {}  // Initialize timeout with default or provided value

NaluUdpReceiver::NaluUdpReceiver(const NaluUdpReceiverParams& params)
: NaluUdpReceiver(params.address, params.port, params.buffer_size, params.max_packet_size, params.timeout_sec) {}

NaluUdpReceiver::~NaluUdpReceiver() {
    stop();
}

void NaluUdpReceiver::initSocket() {
    socket_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd_ < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_);
    inet_pton(AF_INET, address_.c_str(), &server_addr.sin_addr);

    if (bind(socket_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        throw std::runtime_error("Failed to bind socket");
    }

    // Apply timeout if specified
    if (timeout_sec_ > 0) {
        struct timeval timeout;
        timeout.tv_sec = timeout_sec_;
        timeout.tv_usec = 0;
        setsockopt(socket_fd_, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    }
}




void NaluUdpReceiver::start() {
    if (running_) return;

    running_ = true;
    receiver_thread_ = std::thread(&NaluUdpReceiver::receiveLoop, this);
}

void NaluUdpReceiver::stop() {
    if (!running_) return;

    running_ = false;

    // Close the socket before joining the thread to force `recvfrom()` to exit
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
        initSocket();

        uint8_t* UDP_packet_buffer = new uint8_t[max_packet_size_];

        while (running_) {
            sockaddr_in client_addr{};
            socklen_t client_addr_len = sizeof(client_addr);

            ssize_t received_bytes = recvfrom(socket_fd_, UDP_packet_buffer, max_packet_size_, 0,
                                              (struct sockaddr*)&client_addr, &client_addr_len);

            // If recvfrom times out or an error occurs, check if we should exit
            if (received_bytes < 0) {
                if (!running_) break;  // Exit loop when stopping
                continue;
            }

            // Check for malformed packets
            if (received_bytes < 16) {
                std::cerr << "Malformed packet: too small (" << received_bytes << " bytes received)" << std::endl;
                continue;
            }

            // Extract payload size
            uint16_t payload_size;
            std::memcpy(&payload_size, UDP_packet_buffer, sizeof(uint16_t));
            payload_size = ntohs(payload_size);

            if (payload_size != static_cast<uint16_t>(received_bytes - 16)) {
                std::cerr << "Malformed packet: expected payload size " << payload_size
                          << ", but received " << (received_bytes - 16) << " bytes." << std::endl;
                continue;
            }

            // Append to buffer
            try {
                nalu_udp_data_buffer_.append(UDP_packet_buffer + 16, payload_size);
            } catch (const std::overflow_error& e) {
                std::cerr << "Buffer overflow: " << e.what() << std::endl;
            }
        }

        delete[] UDP_packet_buffer;
    } catch (const std::exception& e) {
        std::cerr << "Receiver thread error: " << e.what() << std::endl;
    }
}


NaluUdpDataBuffer& NaluUdpReceiver::getDataBuffer() {
    return nalu_udp_data_buffer_;
}
