#include "nalu_udp_receiver.h"
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include "nalu_event_collector_logger.h"

NaluUdpReceiver::NaluUdpReceiver(const std::string& address, uint16_t port,
                                 size_t buffer_size, size_t max_packet_size,
                                 int timeout_sec)
    : address_(address),
      port_(port),
      max_packet_size_(max_packet_size),
      nalu_udp_data_buffer_(buffer_size),
      socket_fd_(-1),
      running_(false),
      timeout_sec_(timeout_sec) {
    NaluEventCollectorLogger::debug("NaluUdpReceiver initialized with address: " + address + " and port: " + std::to_string(port));
}

NaluUdpReceiver::NaluUdpReceiver(const NaluUdpReceiverParams& params)
    : NaluUdpReceiver(params.address, params.port, params.buffer_size,
                      params.max_packet_size, params.timeout_sec) {
    NaluEventCollectorLogger::debug("NaluUdpReceiver initialized with parameters from NaluUdpReceiverParams.");
}

NaluUdpReceiver::~NaluUdpReceiver() {
    NaluEventCollectorLogger::debug("Destroying NaluUdpReceiver.");
    stop();
}

void NaluUdpReceiver::initSocket() {
    NaluEventCollectorLogger::debug("Initializing socket...");
    socket_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd_ < 0) {
        NaluEventCollectorLogger::error("Failed to create socket");
        throw std::runtime_error("Failed to create socket");
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_);
    inet_pton(AF_INET, address_.c_str(), &server_addr.sin_addr);

    if (bind(socket_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        NaluEventCollectorLogger::error("Failed to bind to socket");
        throw std::runtime_error("Failed to bind socket");
    }

    if (timeout_sec_ > 0) {
        struct timeval timeout {};
        timeout.tv_sec = timeout_sec_;
        timeout.tv_usec = 0;
        setsockopt(socket_fd_, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    }

    NaluEventCollectorLogger::debug("Socket initialization completed.");
}

void NaluUdpReceiver::start() {
    if (running_) {
        NaluEventCollectorLogger::debug("Receiver already running, start call ignored.");
        return;
    }

    NaluEventCollectorLogger::debug("Starting NaluUdpReceiver...");
    running_ = true;
    receiver_thread_ = std::thread(&NaluUdpReceiver::receiveLoop, this);
    NaluEventCollectorLogger::debug("Receiver started.");
}

void NaluUdpReceiver::stop() {
    if (!running_) {
        NaluEventCollectorLogger::debug("Receiver is not running, stop call ignored.");
        return;
    }

    NaluEventCollectorLogger::debug("Stopping NaluUdpReceiver...");
    running_ = false;

    if (socket_fd_ >= 0) {
        close(socket_fd_);
        socket_fd_ = -1;
        NaluEventCollectorLogger::debug("Socket closed.");
    }

    if (receiver_thread_.joinable()) {
        receiver_thread_.join();
        NaluEventCollectorLogger::debug("Receiver thread joined.");
    }

    NaluEventCollectorLogger::debug("Receiver stopped.");
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

            if (received_bytes < 0) {
                NaluEventCollectorLogger::debug("Reached timeout.");
                if (!running_) {
                    NaluEventCollectorLogger::debug("Ending receive loop.");
                    break;
                }
                continue;
            }

            if (received_bytes < 16) {
                NaluEventCollectorLogger::warning("Malformed packet: too small (" + std::to_string(received_bytes) + " bytes received)");
                continue;
            }

            uint16_t payload_size;
            std::memcpy(&payload_size, UDP_packet_buffer, sizeof(uint16_t));
            payload_size = ntohs(payload_size);

            if (payload_size != static_cast<uint16_t>(received_bytes - 16)) {
                NaluEventCollectorLogger::warning("Malformed packet: expected payload size " +
                                                  std::to_string(payload_size) + ", but received " +
                                                  std::to_string(received_bytes - 16) + " bytes.");
                continue;
            }

            nalu_udp_data_buffer_.append(UDP_packet_buffer + 16, payload_size);
        }

        delete[] UDP_packet_buffer;
    } catch (const std::exception& e) {
        NaluEventCollectorLogger::error("Receiver thread error: " + std::string(e.what()));
    }
}

NaluUdpDataBuffer& NaluUdpReceiver::getDataBuffer() {
    return nalu_udp_data_buffer_;
}
