/**
 * @file collection_strategy.h
 * @brief Extensible packet-stream to event collection interface.
 */

#pragma once

#include <vector>

#include "nalu_event_collector/collector/event_buffer.h"
#include "nalu_event_collector/data/packet.h"

namespace nalu_event_collector {

/**
 * @brief Defines how a stream of parsed packets is grouped into output events.
 *
 * Implementations own their event buffer. `poll()` advances time-dependent
 * state even when no packets arrive, while `flush()` publishes any valid
 * partially filled output container during an orderly shutdown.
 */
class CollectionStrategy {
   public:
    virtual ~CollectionStrategy() = default;

    /** @brief Consume one ordered batch of parsed packets. */
    virtual void collect(const std::vector<Packet>& packets) = 0;

    /** @brief Advance deadlines without forcing an incomplete output. */
    virtual void poll() = 0;

    /** @brief Publish valid buffered output before shutdown. */
    virtual void flush() = 0;

    /** @brief Access events produced by this strategy. */
    virtual EventBuffer& event_buffer() = 0;
};

}  // namespace nalu_event_collector
