#include <cstring>
#include <functional>
#include <iostream>
#include <stdexcept>

#include "nalu_event_collector/collector/event_builder.h"
#include "nalu_event_collector/config/event_builder_config.h"
using namespace nalu_event_collector;
namespace {
void require(bool b, const char *message) {
    if (!b)
        throw std::runtime_error(message);
}
void throws(const std::function<void()> &f) {
    bool failed = false;
    try {
        f();
    } catch (const std::exception &) {
        failed = true;
    }
    require(failed, "Expected invalid operation to fail");
}
Packet packet(int channel, uint32_t tick, int window) {
    Packet p;
    p.header = 0xaaaa;
    p.footer = 0xffff;
    p.channel = channel;
    p.trigger_time = tick;
    p.logical_position = window;
    p.physical_position = window;
    for (int i = 0; i < 64; ++i)
        p.raw_samples[i] = uint8_t(channel + window + i);
    return p;
}
EventBuilderConfig config() {
    EventBuilderConfig c;
    c.event_source.channels = {0, 31, 63};
    c.event_source.windows = 8;
    c.event_source.trigger_type = "ext";
    c.event_source.fail_on_data_loss = true;
    c.settings.collection_strategies.trigger_time.time_threshold = 0;
    c.settings.collection_strategies.trigger_time.event_completion_time_us =
        1'000'000;
    return c;
}
void compare_paths(bool interleaved) {
    auto c = config();
    EventBuilder whole(c), fragmented(c);
    std::vector<Packet> input;
    for (uint32_t tick = 100; tick < 228; ++tick) {
        if (interleaved) {
            for (int w = 0; w < 8; ++w)
                for (int ch : {0, 31, 63})
                    input.push_back(packet(ch, tick, w));
        } else
            for (int ch : {0, 31, 63})
                for (int w = 0; w < 8; ++w)
                    input.push_back(packet(ch, tick, w));
    }
    whole.collect_events(input);
    for (size_t offset = 0; offset < input.size();) {
        size_t count =
            std::min<size_t>((offset % 11) + 1, input.size() - offset);
        fragmented.collect_events(std::vector<Packet>(
            input.begin() + offset, input.begin() + offset + count));
        offset += count;
    }
    auto &a = whole.get_event_buffer().get_events();
    auto &b = fragmented.get_event_buffer().get_events();
    require(a.size() == 128 && b.size() == a.size(), "Lost or split event");
    for (size_t i = 0; i < a.size(); ++i) {
        require(a[i]->is_event_complete() && b[i]->is_event_complete(),
                "Incomplete event");
        require(a[i]->get_size() == b[i]->get_size(), "Event size mismatch");
        std::vector<char> x(a[i]->get_size()), y(b[i]->get_size());
        a[i]->serialize_to_buffer(x.data());
        b[i]->serialize_to_buffer(y.data());
        require(x == y, "Fragmented and full-batch payloads differ");
    }
}
void completion_and_boundaries() {
    Event e(0xbbbb, 0x10, 0, 100, 10, 23843000, 10000, sizeof(Packet), 0,
            0xeeee, 8, (1ULL << 63) | 1, 2);
    e.add_packet(packet(0, 100, 0));
    require(!e.is_event_complete(), "Partial waveform complete");
    e.add_packet(packet(0, 100, 1));
    require(!e.is_event_complete(), "Missing channel complete");
    std::vector<Packet> high{packet(63, 100, 0), packet(63, 100, 1)};
    e.add_packet_group(high);
    require(e.is_event_complete(), "All channels not complete");
    e.add_packet(packet(0, 100, 2));
    require(!e.is_event_complete(), "Overfilled channel still complete");
    Event changed(0xbbbb, 0x10, 0, 100, 10, 23843000, 10000, sizeof(Packet), 0,
                  0xeeee, 4, 1, 2);
    changed.add_packet(packet(0, 100, 0));
    changed.header.num_windows = 1;
    require(changed.has_complete_channel_groups(),
            "Public header override ignored");
    changed.header.num_windows = 2;
    require(!changed.has_complete_channel_groups(),
            "Cached state stale after override");
    changed.add_packet(packet(0, 100, 1));
    require(changed.has_complete_channel_groups(),
            "Cached state failed to update");
    Event small(0xbbbb, 0x10, 0, 100, 10, 23843000, 10000, sizeof(Packet), 0,
                0xeeee, 1, 1, 2);
    throws([&] {
        small.add_packet_group({packet(0, 100, 0), packet(0, 100, 1)});
    });
    require(small.header.num_packets == 0,
            "Capacity failure partially appended group");
    EventBuilder builder(config());
    uint32_t index = 0;
    auto &buffer = builder.get_event_buffer();
    auto bad = std::vector<Packet>(8, packet(0, 100, 0));
    bad.back().channel = 31;
    throws([&] { buffer.add_packet_group(bad, index); });
    require(buffer.get_events().empty(), "Invalid group mutated output");
    auto unknown = config();
    EventBuilder unconfigured(unknown);
    std::vector<Packet> invalid;
    for (int w = 0; w < 8; ++w)
        invalid.push_back(packet(64, 100, w));
    throws([&] { unconfigured.collect_events(invalid); });
    auto c = config();
    EventBuilder duplicates(c);
    auto same = std::vector<Packet>(8, packet(0, 100, 0));
    throws([&] { duplicates.collect_events(same); });
    auto corrupt = std::vector<Packet>{};
    for (int w = 0; w < 8; ++w)
        corrupt.push_back(packet(0, 100, w));
    corrupt.back().info = 1;
    EventBuilder errors(c);
    throws([&] { errors.collect_events(corrupt); });
}
} // namespace
int main() {
    compare_paths(false);
    compare_paths(true);
    completion_and_boundaries();
    std::cout
        << "Contiguous/fragmented/interleaved byte equivalence, high channels, "
           "completion transitions, capacity atomicity and validation passed\n";
}
