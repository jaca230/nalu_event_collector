/**
 * @file udp_receiver_config.h
 * @brief Configuration for UDP reception and buffering.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace nalu_event_collector {

/**
 * @brief Socket and buffer configuration for UdpReceiver.
 */
struct UdpReceiverConfig {
    /** @brief Local bind address. */
    std::string address = "127.0.0.1";

    /** @brief Local bind port. */
    uint16_t port = 9000;

    /** @brief Capacity of the raw byte buffer used after UDP header stripping. */
    size_t buffer_size = 1024 * 1024 * 100;

    /** @brief Maximum UDP datagram size expected from the sender. */
    size_t max_packet_size = 1040;

    /** @brief Receive timeout in seconds. */
    int timeout_sec = 10;
};

}  // namespace nalu_event_collector
