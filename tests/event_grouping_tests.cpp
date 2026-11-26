#include <chrono>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "nalu_event_collector/collector/collector.h"
#include "nalu_event_collector/collector/event_builder.h"
#include "nalu_event_collector/config/collector_config.h"
#include "nalu_event_collector/data/event.h"
#include "nalu_event_collector/data/packet.h"

namespace {

using nalu_event_collector::CollectionStrategy;
using nalu_event_collector::Collector;
using nalu_event_collector::CollectorConfig;
using nalu_event_collector::Event;
using nalu_event_collector::EventBuffer;
using nalu_event_collector::EventBuilder;
using nalu_event_collector::EventBuilderConfig;
using nalu_event_collector::Packet;

class TestCollectionStrategy final : public CollectionStrategy {
   public:
    explicit TestCollectionStrategy(int marker) : marker(marker), buffer(4) {}

    void collect(const std::vector<Packet>& packets) override {
        collected_packets += packets.size();
    }
    void poll() override { ++poll_count; }
    void flush() override { ++flush_count; }
    EventBuffer& event_buffer() override { return buffer; }

    int marker;
    size_t collected_packets = 0;
    size_t poll_count = 0;
    size_t flush_count = 0;
    EventBuffer buffer;
};

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

std::vector<Packet> group(uint8_t channel, uint32_t trigger_time,
                          uint16_t& parser_index, size_t count = 2) {
    std::vector<Packet> packets;
    packets.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        packets.push_back(packet(channel, trigger_time, parser_index++));
    }
    return packets;
}

void append(std::vector<Packet>& destination,
            const std::vector<Packet>& source) {
    destination.insert(destination.end(), source.begin(), source.end());
}

EventBuilderConfig external_config() {
    EventBuilderConfig config;
    auto& strategy = config.settings.collection_strategies.trigger_time;
    config.event_source.channels = {0, 1};
    config.event_source.windows = 2;
    config.event_source.trigger_type = "ext";
    strategy.time_threshold = 10;
    strategy.max_lookback = 4;
    strategy.event_completion_time_us = 1'000'000;
    return config;
}

EventBuilderConfig self_config() {
    EventBuilderConfig config = external_config();
    auto& strategy = config.settings.collection_strategies.trigger_time;
    config.event_source.channels = {0, 2};
    config.event_source.trigger_type = "self";
    strategy.event_completion_time_us = 0;
    return config;
}

EventBuilderConfig time_batch_config(std::string trigger_type = "ext") {
    EventBuilderConfig config =
        trigger_type == "self" ? self_config() : external_config();
    config.settings.collection_strategy = "time_batch";
    config.settings.collection_strategies.time_batch.trigger_events =
        config.settings.collection_strategies.trigger_time;
    config.settings.collection_strategies.time_batch.duration_us = 1'000'000;
    config.settings.collection_strategies.time_batch.max_packets = 64;
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
        require(event->header.num_packets == 4,
                "event must contain two complete groups");
        require(event->get_channel_packet_count(0) == 2,
                "channel 0 group was split");
        require(event->get_channel_packet_count(1) == 2,
                "channel 1 group was split");
        require(event->is_event_complete(),
                "balanced external event should be complete");
    }
}

void test_partial_groups_only_publish_when_complete() {
    EventBuilderConfig config = external_config();
    config.event_source.channels = {0};
    EventBuilder builder(config);
    uint16_t parser_index = 0;

    builder.collect_events({packet(0, 500, parser_index++)});
    require(builder.get_event_buffer().get_events().empty(),
            "a partial channel group must not create an event");

    builder.collect_events({packet(0, 500, parser_index++)});
    auto& events = builder.get_event_buffer().get_events();
    require(events.size() == 1,
            "the completed channel group should create one event");
    require(events.front()->header.num_packets == 2,
            "completed group has the wrong size");
}

void test_external_completion_requires_each_channel() {
    Event event(0xBBBB, 0x10, 0, 100, 10, 23'843'000, 1'000'000, 80, 0, 0xEEEE,
                16, 0x3, 2, false, 4, true);

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

    require(events.size() == 1,
            "one complete channel group is a valid self-trigger event");
    require(events.front()->header.channel_mask == (1ULL << 2),
            "self-trigger mask must contain only observed channels");
    require(events.front()->header.num_packets == 2,
            "self-trigger event should contain one complete channel group");
    require(
        events.front()->max_packets == 4,
        "event storage should be bounded by windows times allowed channels");
    require(events.front()->has_complete_channel_groups(),
            "every observed self-trigger channel must have exactly windows "
            "packets");
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
    require(
        collector.get_event_builder().get_event_buffer().get_events().empty(),
        "a transferred event must no longer be retained by the collector");
}

void test_partial_observed_self_trigger_channel_is_invalid() {
    Event event(0xBBBB, 0x20, 0, 700, 10, 23'843'000, 0, 80, 0, 0xEEEE, 16, 0x5,
                2, true);

    event.add_packet(packet(2, 700, 0));
    require(event.header.channel_mask == (1ULL << 2),
            "a self-trigger mask must track the partially observed channel");
    require(
        !event.has_complete_channel_groups(),
        "an observed channel with fewer than windows packets must be invalid");
    require(!event.is_event_complete(),
            "timeout must not complete a malformed self-trigger event");
}

void test_incomplete_hole_does_not_duplicate_later_event() {
    CollectorConfig config;
    config.event_builder = external_config();
    config.event_builder.settings.collection_strategies.trigger_time
        .event_completion_time_us = 0;
    Collector collector(config);

    uint16_t parser_index = 0;
    std::vector<Packet> packets;
    append(packets,
           group(0, 100,
                 parser_index));  // A remains incomplete: channel 1 is missing.
    append(packets, group(0, 200, parser_index));
    append(packets, group(1, 201, parser_index));  // B is complete.
    collector.get_event_builder().collect_events(packets);

    auto first = collector.get_data().second;
    require(
        first.size() == 1 && first.front()->header.reference_time == 200,
        "complete event B should be returned after expired event A is dropped");
    collector.clear_events();

    auto second = collector.get_data().second;
    require(second.empty(), "complete event B must not be returned twice");
}

void test_time_batch_combines_complete_external_trigger_events() {
    EventBuilder builder(time_batch_config());
    uint16_t parser_index = 0;
    std::vector<Packet> packets;
    append(packets, group(0, 100, parser_index));
    append(packets, group(1, 101, parser_index));
    append(packets, group(0, 200, parser_index));
    append(packets, group(1, 201, parser_index));

    builder.collect_events(packets);
    require(builder.get_event_buffer().get_events().empty(),
            "an open time batch must not be published before its deadline");
    builder.flush();

    auto& batches = builder.get_event_buffer().get_events();
    require(batches.size() == 1,
            "two trigger events should share one time batch");
    require(batches.front()->header.num_packets == 8,
            "the time batch should contain both four-packet trigger events");
    require(batches.front()->is_event_complete(),
            "a flushed time batch must be ready");

    size_t first_trigger_packets = 0;
    size_t second_trigger_packets = 0;
    for (size_t i = 0; i < batches.front()->header.num_packets; ++i) {
        const uint32_t trigger_time = batches.front()->packets[i].trigger_time;
        if (trigger_time == 100 || trigger_time == 101) {
            ++first_trigger_packets;
        } else if (trigger_time == 200 || trigger_time == 201) {
            ++second_trigger_packets;
        }
    }
    require(first_trigger_packets == 4 && second_trigger_packets == 4,
            "constituent trigger timestamps must survive time batching");
}

void test_zero_duration_time_batch_closes_each_collection_cycle() {
    EventBuilderConfig config = time_batch_config();
    config.event_source.channels = {0};
    config.settings.collection_strategies.time_batch.duration_us = 0;
    EventBuilder builder(config);
    uint16_t parser_index = 0;

    builder.collect_events(group(0, 100, parser_index));
    builder.collect_events(group(0, 200, parser_index));

    auto& batches = builder.get_event_buffer().get_events();
    require(batches.size() == 2,
            "zero-duration batching should publish one batch per collect call");
    require(
        batches[0]->header.num_packets == 2 &&
            batches[1]->header.num_packets == 2,
        "each zero-duration batch should contain one complete trigger event");
}

void test_time_batch_publishes_on_fixed_deadline_without_new_packets() {
    EventBuilderConfig config = time_batch_config();
    config.event_source.channels = {0};
    config.settings.collection_strategies.time_batch.duration_us = 500;
    EventBuilder builder(config);
    uint16_t parser_index = 0;

    builder.collect_events(group(0, 250, parser_index));
    require(builder.get_event_buffer().get_events().empty(),
            "the time batch should remain open before its deadline");
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    builder.poll();

    require(
        builder.get_event_buffer().get_events().size() == 1,
        "poll should publish an expired batch even when no new packets arrive");
}

void test_time_batch_preserves_self_trigger_grouping() {
    EventBuilder builder(time_batch_config("self"));
    uint16_t parser_index = 0;
    std::vector<Packet> packets;
    append(packets, group(0, 300, parser_index));
    append(packets, group(0, 400, parser_index));
    builder.collect_events(packets);
    builder.flush();

    auto& batches = builder.get_event_buffer().get_events();
    require(batches.size() == 1,
            "self-trigger events should be combined into one batch");
    require(batches.front()->header.num_packets == 4,
            "both self-trigger channel groups should be retained");
    require(batches.front()->header.channel_mask == 1,
            "the batch mask should report the observed self-trigger channel");
}

void test_time_batch_drops_incomplete_external_trigger_event() {
    EventBuilder builder(time_batch_config());
    uint16_t parser_index = 0;
    builder.collect_events(group(0, 500, parser_index));
    builder.flush();
    require(builder.get_event_buffer().get_events().empty(),
            "an incomplete external trigger must not enter a time batch");
}

void test_time_batch_respects_packet_capacity_at_trigger_boundaries() {
    EventBuilderConfig config = time_batch_config();
    config.settings.collection_strategies.time_batch.max_packets = 4;
    EventBuilder builder(config);
    uint16_t parser_index = 0;
    std::vector<Packet> packets;
    append(packets, group(0, 600, parser_index));
    append(packets, group(1, 601, parser_index));
    append(packets, group(0, 700, parser_index));
    append(packets, group(1, 701, parser_index));
    builder.collect_events(packets);
    builder.flush();

    auto& batches = builder.get_event_buffer().get_events();
    require(batches.size() == 2,
            "capacity should split batches between trigger events");
    require(batches[0]->header.num_packets == 4 &&
                batches[1]->header.num_packets == 4,
            "capacity splitting must not divide a trigger event");
}

void test_unknown_collection_strategy_is_rejected() {
    EventBuilderConfig config = external_config();
    config.settings.collection_strategy = "not_a_strategy";
    try {
        EventBuilder builder(config);
        static_cast<void>(builder);
        require(false, "an unknown collection strategy should throw");
    } catch (const std::invalid_argument& error) {
        require(std::string(error.what()).find("trigger_time") !=
                        std::string::npos &&
                    std::string(error.what()).find("time_batch") !=
                        std::string::npos,
                "the strategy error should list supported values");
    }
}

void test_application_defined_strategy_can_own_its_config() {
    EventBuilderConfig global_config;
    auto strategy = std::make_unique<TestCollectionStrategy>(42);
    auto* strategy_view = strategy.get();
    EventBuilder builder(global_config, std::move(strategy));

    builder.collect_events({packet(0, 10, 0), packet(1, 11, 1)});
    builder.poll();
    builder.flush();

    require(strategy_view->marker == 42,
            "an injected strategy should retain its application-owned "
            "configuration");
    require(strategy_view->collected_packets == 2 &&
                strategy_view->poll_count == 1 &&
                strategy_view->flush_count == 1,
            "EventBuilder must delegate collection lifecycle calls to a custom "
            "strategy");
}

}  // namespace

int main() {
    test_channel_groups_are_indivisible();
    test_partial_groups_only_publish_when_complete();
    test_external_completion_requires_each_channel();
    test_self_trigger_requires_windows_only_for_observed_channels();
    test_take_data_transfers_event_ownership();
    test_partial_observed_self_trigger_channel_is_invalid();
    test_incomplete_hole_does_not_duplicate_later_event();
    test_time_batch_combines_complete_external_trigger_events();
    test_zero_duration_time_batch_closes_each_collection_cycle();
    test_time_batch_publishes_on_fixed_deadline_without_new_packets();
    test_time_batch_preserves_self_trigger_grouping();
    test_time_batch_drops_incomplete_external_trigger_event();
    test_time_batch_respects_packet_capacity_at_trigger_boundaries();
    test_unknown_collection_strategy_is_rejected();
    test_application_defined_strategy_can_own_its_config();
    return 0;
}
