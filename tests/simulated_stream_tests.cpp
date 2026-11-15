#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include "nalu_event_collector/collector/event_builder.h"
#include "nalu_event_collector/config/event_builder_config.h"
#include "nalu_event_collector/config/packet_parser_config.h"
#include "nalu_event_collector/parsing/packet_parser.h"

namespace {

using nalu_event_collector::EventBuilder;
using nalu_event_collector::EventBuilderConfig;
using nalu_event_collector::Packet;
using nalu_event_collector::PacketParser;
using nalu_event_collector::PacketParserConfig;

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

std::vector<uint8_t> raw_packet(uint8_t channel,
                                uint32_t trigger_time,
                                uint16_t logical_position = 0) {
    std::vector<uint8_t> bytes(74, 0);
    bytes[0] = 0x0E;
    bytes[1] = channel;

    const uint16_t upper_time = static_cast<uint16_t>(trigger_time >> 12U);
    const uint16_t lower_time = static_cast<uint16_t>(trigger_time & 0x0FFFU);
    bytes[2] = static_cast<uint8_t>(upper_time >> 8U);
    bytes[3] = static_cast<uint8_t>(upper_time);
    bytes[4] = static_cast<uint8_t>(lower_time >> 8U);
    bytes[5] = static_cast<uint8_t>(lower_time);
    bytes[6] = static_cast<uint8_t>((logical_position >> 2U) & 0x3FU);
    bytes[7] = static_cast<uint8_t>((logical_position & 0x3U) << 6U);

    for (size_t i = 0; i < 64; ++i) {
        bytes[8 + i] = static_cast<uint8_t>((trigger_time + channel + i) & 0xFFU);
    }
    bytes[72] = 0xFA;
    bytes[73] = 0x5A;
    return bytes;
}

void append(std::vector<uint8_t>& destination, const std::vector<uint8_t>& source) {
    destination.insert(destination.end(), source.begin(), source.end());
}

void append_channel_group(std::vector<uint8_t>& stream,
                          uint8_t channel,
                          uint32_t trigger_time,
                          size_t windows) {
    for (size_t window = 0; window < windows; ++window) {
        append(stream,
               raw_packet(channel, trigger_time, static_cast<uint16_t>(window)));
    }
}

void append_external_trigger(std::vector<uint8_t>& stream,
                             uint32_t trigger_time,
                             size_t windows = 2) {
    append_channel_group(stream, 0, trigger_time, windows);
    append_channel_group(stream, 1, trigger_time + 1, windows);
}

PacketParser parser() {
    PacketParserConfig config;
    config.check_packet_integrity = true;
    return PacketParser(config);
}

EventBuilderConfig trigger_config() {
    EventBuilderConfig config;
    config.max_events_in_buffer = 4096;
    auto& strategy = config.collection_strategies.trigger_time;
    strategy.channels = {0, 1};
    strategy.trigger_type = "ext";
    strategy.windows = 2;
    strategy.time_threshold = 10;
    strategy.event_completion_time_us = 1'000'000;
    strategy.max_lookback = 8;
    return config;
}

EventBuilderConfig batch_config() {
    EventBuilderConfig config = trigger_config();
    config.collection_strategy = "time_batch";
    auto& batch = config.collection_strategies.time_batch;
    batch.duration_us = 0;
    batch.max_packets = 4096;
    batch.trigger_events = config.collection_strategies.trigger_time;
    return config;
}

std::vector<Packet> parse_fragmented(PacketParser& packet_parser,
                                     const std::vector<uint8_t>& stream,
                                     const std::vector<size_t>& chunk_sizes) {
    std::vector<Packet> packets;
    size_t offset = 0;
    size_t chunk_index = 0;
    while (offset < stream.size()) {
        const size_t chunk_size = std::min(
            chunk_sizes[chunk_index++ % chunk_sizes.size()], stream.size() - offset);
        std::vector<uint8_t> chunk(stream.begin() + offset,
                                   stream.begin() + offset + chunk_size);
        auto parsed = packet_parser.process_stream(chunk);
        packets.insert(packets.end(), parsed.begin(), parsed.end());
        offset += chunk_size;
    }
    return packets;
}

void test_parser_accepts_arbitrarily_fragmented_stream() {
    std::vector<uint8_t> stream;
    append(stream, raw_packet(3, 0x123456, 0));
    append(stream, raw_packet(7, 0x654321, 1));
    append(stream, raw_packet(9, 0x0ABCDE, 2));

    auto packet_parser = parser();
    const auto packets = parse_fragmented(
        packet_parser, stream, {1, 2, 5, 3, 17, 4, 71, 6, 9});

    require(packets.size() == 3, "fragmented stream should produce three packets");
    require(packets[0].channel == 3 && packets[0].trigger_time == 0x123456,
            "first fragmented packet decoded incorrectly");
    require(packets[1].channel == 7 && packets[1].trigger_time == 0x654321,
            "second fragmented packet decoded incorrectly");
    require(packets[2].channel == 9 && packets[2].trigger_time == 0x0ABCDE,
            "third fragmented packet decoded incorrectly");
}

void test_fragmented_threshold_boundary_stream_keeps_groups_indivisible() {
    std::vector<uint8_t> stream;
    append_channel_group(stream, 0, 100, 2);
    append_channel_group(stream, 0, 110, 2);
    append_channel_group(stream, 1, 111, 2);
    append_channel_group(stream, 1, 101, 2);

    auto packet_parser = parser();
    EventBuilder builder(trigger_config());
    size_t offset = 0;
    const std::vector<size_t> chunks = {13, 149, 7, 81, 2, 211};
    size_t chunk_index = 0;
    while (offset < stream.size()) {
        const size_t size =
            std::min(chunks[chunk_index++ % chunks.size()], stream.size() - offset);
        std::vector<uint8_t> chunk(stream.begin() + offset, stream.begin() + offset + size);
        const auto packets = packet_parser.process_stream(chunk);
        if (!packets.empty()) {
            builder.collect_events(packets);
        }
        offset += size;
    }

    auto& events = builder.get_event_buffer().get_events();
    require(events.size() == 2, "threshold-boundary stream should produce two events");
    for (const auto& event : events) {
        require(event->header.num_packets == 4,
                "a simulated channel group was split between events");
        require(event->get_channel_packet_count(0) == 2 &&
                    event->get_channel_packet_count(1) == 2,
                "simulated event has imbalanced channel groups");
    }
}

void test_large_simulated_trigger_stream() {
    constexpr size_t trigger_count = 1000;
    std::vector<uint8_t> stream;
    stream.reserve(trigger_count * 4 * 74);
    for (size_t trigger = 0; trigger < trigger_count; ++trigger) {
        append_external_trigger(stream, static_cast<uint32_t>(1000 + trigger * 100));
    }

    auto packet_parser = parser();
    EventBuilder builder(trigger_config());
    size_t offset = 0;
    const std::vector<size_t> chunks = {4093, 17, 8191, 73, 1024, 5};
    size_t chunk_index = 0;
    while (offset < stream.size()) {
        const size_t size =
            std::min(chunks[chunk_index++ % chunks.size()], stream.size() - offset);
        std::vector<uint8_t> chunk(stream.begin() + offset, stream.begin() + offset + size);
        const auto packets = packet_parser.process_stream(chunk);
        if (!packets.empty()) {
            builder.collect_events(packets);
        }
        offset += size;
    }

    auto& events = builder.get_event_buffer().get_events();
    require(events.size() == trigger_count,
            "large simulated stream lost or merged trigger events");
    for (size_t index = 0; index < events.size(); ++index) {
        require(events[index]->header.num_packets == 4,
                "large simulated stream produced a malformed event");
        require(events[index]->header.reference_time == 1000 + index * 100,
                "large simulated stream reordered trigger events");
    }
}

void test_simulated_readout_cycles_form_time_batches() {
    auto packet_parser = parser();
    EventBuilder builder(batch_config());

    constexpr size_t cycles = 10;
    constexpr size_t triggers_per_cycle = 10;
    for (size_t cycle = 0; cycle < cycles; ++cycle) {
        std::vector<uint8_t> readout_cycle;
        for (size_t trigger = 0; trigger < triggers_per_cycle; ++trigger) {
            const uint32_t timestamp =
                static_cast<uint32_t>(10'000 + (cycle * triggers_per_cycle + trigger) * 100);
            append_external_trigger(readout_cycle, timestamp);
        }
        const auto packets = packet_parser.process_stream(readout_cycle);
        require(packets.size() == triggers_per_cycle * 4,
                "simulated readout cycle parsed the wrong packet count");
        builder.collect_events(packets);
    }

    auto& batches = builder.get_event_buffer().get_events();
    require(batches.size() == cycles,
            "zero-duration strategy should produce one batch per simulated readout cycle");
    for (const auto& batch : batches) {
        require(batch->header.num_packets == triggers_per_cycle * 4,
                "time batch contains the wrong number of constituent trigger packets");
        require(batch->is_event_complete(), "published time batch should be complete");
    }
}

void test_dropped_packet_does_not_contaminate_next_time_batch_event() {
    std::vector<uint8_t> stream;
    append_channel_group(stream, 0, 100, 2);
    append(stream, raw_packet(1, 101, 0));  // One window from this trigger is lost.
    append_external_trigger(stream, 200);

    EventBuilderConfig config = batch_config();
    config.collection_strategies.time_batch.trigger_events.event_completion_time_us = 0;
    EventBuilder builder(config);
    auto packet_parser = parser();
    const auto packets = packet_parser.process_stream(stream);
    builder.collect_events(packets);

    auto& batches = builder.get_event_buffer().get_events();
    require(batches.size() == 1, "complete trigger after a loss should still be batched");
    require(batches.front()->header.num_packets == 4,
            "incomplete trigger packets leaked into the next batch");
    for (size_t i = 0; i < batches.front()->header.num_packets; ++i) {
        require(batches.front()->packets[i].trigger_time >= 200,
                "dropped-packet trigger contaminated the completed batch");
    }
}

}  // namespace

int main() {
    test_parser_accepts_arbitrarily_fragmented_stream();
    test_fragmented_threshold_boundary_stream_keeps_groups_indivisible();
    test_large_simulated_trigger_stream();
    test_simulated_readout_cycles_form_time_batches();
    test_dropped_packet_does_not_contaminate_next_time_batch_event();
    return 0;
}
