/**
 * @file collector_config.h
 * @brief Aggregate configuration for constructing a Collector.
 */

#pragma once

#include <chrono>

#include "nalu_event_collector/config/event_builder_config.h"
#include "nalu_event_collector/config/packet_parser_config.h"
#include "nalu_event_collector/config/udp_receiver_config.h"

namespace nalu_event_collector {

/**
 * @brief Top-level configuration for the full collection pipeline.
 */
struct CollectorConfig {
    /** @brief Event grouping and buffer settings. */
    EventBuilderConfig event_builder;

    /** @brief UDP socket and byte-buffer settings. */
    UdpReceiverConfig udp_receiver;

    /** @brief Packet decoding settings. */
    PacketParserConfig packet_parser;

    /** @brief Optional sleep inserted between background collection cycles. */
    std::chrono::microseconds sleep_time_us = std::chrono::microseconds(-1);
};

}  // namespace nalu_event_collector
