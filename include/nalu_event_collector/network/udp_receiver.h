/**
 * @file udp_receiver.h
 * @brief UDP socket receiver for incoming Nalu data traffic.
 */

#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>
#include <thread>

#include "nalu_event_collector/config/udp_receiver_config.h"
#include "nalu_event_collector/network/udp_data_buffer.h"

namespace nalu_event_collector {

/**
 * @brief Receives UDP datagrams and forwards payload bytes into a UdpDataBuffer.
 *
 * The receiver strips the fixed transport header currently expected by the
 * collector and exposes the remaining payload stream to downstream parsing
 * code.
 */
class UdpReceiver {
  public:
    /** @brief Construct a receiver from explicit socket parameters. */
    UdpReceiver(const std::string& address,
                uint16_t port,
                size_t buffer_size = 1024 * 1024 * 100,
                size_t max_packet_size = 1040,
                int timeout_sec = 10);

    /** @brief Construct a receiver from a configuration object. */
    explicit UdpReceiver(const UdpReceiverConfig& config);

    /** @brief Stop any running receive loop and release the socket. */
    ~UdpReceiver();

    /** @brief Start the UDP receive thread. */
    void start();

    /** @brief Stop the UDP receive thread and close the socket. */
    void stop();

    /** @brief Access the owned raw byte buffer. */
    UdpDataBuffer& getDataBuffer();

  private:
    void initSocket();
    void receiveLoop();

    std::string address_;
    uint16_t port_;
    int socket_fd_;
    std::thread receiver_thread_;
    std::atomic<bool> running_;
    UdpDataBuffer data_buffer_;
    size_t max_packet_size_;
    int timeout_sec_;
};

}  // namespace nalu_event_collector
