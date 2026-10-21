#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include "nalu_event_collector/collector/collector.h"
#include "nalu_event_collector/collector/event_builder.h"
#include "nalu_event_collector/config/collector_config.h"
#include "nalu_event_collector/data/event.h"
#include "nalu_event_collector/data/packet.h"

namespace {

using nalu_event_collector::Collector;
using nalu_event_collector::CollectorConfig;
using nalu_event_collector::Event;
using nalu_event_collector::EventBuilder;
using nalu_event_collector::EventBuilderConfig;
using nalu_event_collector::Packet;

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

Packet packet(uint8_t channel, uint32_t trigger_time, uint16_t parser_index) {
    Packet value;
    value.header = 0xAAAA;
    value.channel = channel;
    value.trigger_time = trigger_time;
    value.parser_index = parser_index;
    value.footer = 0xFFFF;
    return value;
}

std::vector<Packet> group(uint8_t channel,
                          uint32_t trigger_time,
                          uint16_t& parser_index,
                          size_t count = 2) {
    std::vector<Packet> packets;
    packets.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        packets.push_back(packet(channel, trigger_time, parser_index++));
    }
    return packets;
}

void append(std::vector<Packet>& destination, const std::vector<Packet>& source) {
    destination.insert(destination.end(), source.begin(), source.end());
}

EventBuilderConfig external_config() {
    EventBuilderConfig config;
    config.channels = {0, 1};
    config.windows = 2;
    config.trigger_type = "ext";
    config.time_threshold = 10;
    config.max_lookback = 4;
    config.event_completion_time_us = 1'000'000;
    return config;
}

EventBuilderConfig self_config() {
    EventBuilderConfig config = external_config();
    config.channels = {0, 2};
    config.trigger_type = "self";
    config.event_completion_time_us = 0;
    return config;
}

void test_channel_groups_are_indivisible() {
    EventBuilder builder(external_config());
    uint16_t parser_index = 0;
    std::vector<Packet> packets;

    // This ordering reproduces the threshold-boundary failure: trigger B is
    // close enough to trigger A that packet-by-packet grouping used to split
    // B between A and a new event.
    append(packets, group(0, 100, parser_index));
    append(packets, group(0, 110, parser_index));
    append(packets, group(1, 111, parser_index));
    append(packets, group(1, 101, parser_index));
    builder.collect_events(packets);

    auto& events = builder.get_event_buffer().get_events();
    require(events.size() == 2, "expected exactly two events");
    for (const auto& event : events) {
        require(event->header.num_packets == 4, "event must contain two complete groups");
        require(event->get_channel_packet_count(0) == 2, "channel 0 group was split");
        require(event->get_channel_packet_count(1) == 2, "channel 1 group was split");
        require(event->is_event_complete(), "balanced external event should be complete");
    }
}

void test_partial_and_duplicate_groups_are_not_events() {
    EventBuilderConfig config = external_config();
    config.channels = {0};
    EventBuilder builder(config);
    uint16_t parser_index = 0;

    builder.collect_events({packet(0, 500, parser_index++)});
    require(builder.get_event_buffer().get_events().empty(),
            "a partial channel group must not create an event");

    builder.collect_events({packet(0, 500, parser_index++)});
    auto& events = builder.get_event_buffer().get_events();
    require(events.size() == 1, "the completed channel group should create one event");
    require(events.front()->header.num_packets == 2, "completed group has the wrong size");

    builder.collect_events({packet(0, 500, parser_index++)});
    require(events.size() == 1 && events.front()->header.num_packets == 2,
            "a duplicate packet must not reopen or extend a completed group");
}

void test_external_completion_requires_each_channel() {
    Event event(0xBBBB,
                0x10,
                0,
                100,
                10,
                23'843'000,
                1'000'000,
                80,
                0,
                0xEEEE,
                16,
                0x3,
                2,
                false,
                4,
                true);

    event.add_packet(packet(0, 100, 0));
    event.add_packet(packet(0, 100, 1));
    event.add_packet(packet(0, 100, 2));
    event.add_packet(packet(1, 101, 3));
    require(!event.is_event_complete(),
            "total packet count must not hide an imbalanced per-channel event");
}

void test_self_trigger_requires_windows_only_for_observed_channels() {
    CollectorConfig config;
    config.event_builder = self_config();
    Collector collector(config);
    uint16_t parser_index = 0;

    collector.get_event_builder().collect_events(group(2, 700, parser_index));
    auto events = collector.get_data().second;

    require(events.size() == 1, "one complete channel group is a valid self-trigger event");
    require(events.front()->header.channel_mask == (1ULL << 2),
            "self-trigger mask must contain only observed channels");
    require(events.front()->header.num_packets == 2,
            "self-trigger event should contain one complete channel group");
    require(events.front()->max_packets == 4,
            "event storage should be bounded by windows times allowed channels");
    require(events.front()->has_complete_channel_groups(),
            "every observed self-trigger channel must have exactly windows packets");
}

void test_take_data_transfers_event_ownership() {
    CollectorConfig config;
    config.event_builder = self_config();
    Collector collector(config);
    uint16_t parser_index = 0;

    collector.get_event_builder().collect_events(group(0, 800, parser_index));
    auto data = collector.take_data();

    require(data.second.size() == 1, "take_data should return the ready event");
    require(data.second.front()->header.reference_time == 800,
            "the transferred event should remain valid");
    require(collector.get_event_builder().get_event_buffer().get_events().empty(),
            "a transferred event must no longer be retained by the collector");
}

void test_partial_observed_self_trigger_channel_is_invalid() {
    Event event(0xBBBB,
                0x20,
                0,
                700,
                10,
                23'843'000,
                0,
                80,
                0,
                0xEEEE,
                16,
                0x5,
                2,
                true);

    event.add_packet(packet(2, 700, 0));
    require(event.header.channel_mask == (1ULL << 2),
            "a self-trigger mask must track the partially observed channel");
    require(!event.has_complete_channel_groups(),
            "an observed channel with fewer than windows packets must be invalid");
    require(!event.is_event_complete(),
            "timeout must not complete a malformed self-trigger event");
}

void test_incomplete_hole_does_not_duplicate_later_event() {
    CollectorConfig config;
    config.event_builder = external_config();
    config.event_builder.event_completion_time_us = 0;
    Collector collector(config);

    uint16_t parser_index = 0;
    std::vector<Packet> packets;
    append(packets, group(0, 100, parser_index));  // A remains incomplete: channel 1 is missing.
    append(packets, group(0, 200, parser_index));
    append(packets, group(1, 201, parser_index));  // B is complete.
    collector.get_event_builder().collect_events(packets);

    auto first = collector.get_data().second;
    require(first.size() == 1 && first.front()->header.reference_time == 200,
            "complete event B should be returned after expired event A is dropped");
    collector.clear_events();

    auto second = collector.get_data().second;
    require(second.empty(), "complete event B must not be returned twice");
}

}  // namespace

int main() {
    test_channel_groups_are_indivisible();
    test_partial_and_duplicate_groups_are_not_events();
    test_external_completion_requires_each_channel();
    test_self_trigger_requires_windows_only_for_observed_channels();
    test_take_data_transfers_event_ownership();
    test_partial_observed_self_trigger_channel_is_invalid();
    test_incomplete_hole_does_not_duplicate_later_event();
    return 0;
}
