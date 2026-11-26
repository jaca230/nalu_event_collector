#include "nalu_event_collector/collector/collector.h"
#include "nalu_event_collector/simulation/udp_simulator.h"
#include <arpa/inet.h>
#include <chrono>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
using namespace nalu_event_collector;
void require(bool value, const char *why) {
    if (!value)
        throw std::runtime_error(why);
}
void rejects(const std::function<void()> &f, const std::string &text) {
    bool rejected = false;
    try {
        f();
    } catch (const std::exception &e) {
        rejected = true;
        require(std::string(e.what()).find(text) != std::string::npos,
                e.what());
    }
    require(rejected, "Expected failure was not propagated");
}
CollectorConfig config() {
    CollectorConfig c;
    c.event_builder.event_source.channels = {0, 3};
    c.event_builder.event_source.windows = 2;
    c.event_builder.event_source.trigger_type = "ext";
    c.event_builder.event_source.fail_on_data_loss = true;
    c.event_builder.settings.collection_strategies.trigger_time.time_threshold =
        0;
    c.event_builder.settings.collection_strategies.trigger_time
        .event_completion_time_us = 1000000;
    return c;
}
std::vector<Packet> group(unsigned channel, uint32_t tick,
                          unsigned windows = 2) {
    std::vector<Packet> packets(windows);
    for (unsigned w = 0; w < windows; ++w) {
        auto &p = packets[w];
        p.channel = channel;
        p.trigger_time = tick;
        p.logical_position = 0; // Firmware 957 does not report this reliably.
        p.physical_position = (60 + w) % 64;
    }
    return packets;
}
uint16_t free_port() {
    const int fd = socket(AF_INET, SOCK_DGRAM, 0);
    require(fd >= 0, "socket");
    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    require(bind(fd, reinterpret_cast<sockaddr *>(&a), sizeof(a)) == 0, "bind");
    socklen_t size = sizeof(a);
    require(getsockname(fd, reinterpret_cast<sockaddr *>(&a), &size) == 0,
            "getsockname");
    const auto port = ntohs(a.sin_port);
    close(fd);
    return port;
}
void firmware_957() {
    auto c = config();
    c.event_builder.event_source.windows = 61;
    for (bool fragmented : {false, true}) {
        EventBuilder b(c.event_builder);
        for (unsigned channel : {0U, 3U}) {
            auto packets = group(channel, 100, 61);
            if (fragmented)
                for (const auto &p : packets)
                    b.collect_events({p});
            else
                b.collect_events(packets);
        }
        b.flush();
        auto &events = b.get_event_buffer().get_events();
        require(events.size() == 1 && events[0]->header.num_packets == 122,
                "957 groups not assembled");
        for (unsigned k = 0; k < 122; ++k)
            require(events[0]->packets[k].logical_position == 0,
                    "Logical metadata modified");
    }
    EventBuilder b(config().event_builder);
    auto p = group(0, 100);
    p[1].physical_position = p[0].physical_position;
    p[1].logical_position = 1;
    rejects([&] { b.collect_events(p); }, "Duplicate window");
}
void final_flush() {
    auto c = config();
    Collector partial(c);
    partial.get_event_builder().collect_events({group(0, 100)[0]});
    rejects([&] { partial.flush(); }, "Incomplete waveform at final flush");
    Collector missing_channel(c);
    missing_channel.get_event_builder().collect_events(group(0, 100));
    rejects([&] { missing_channel.flush(); },
            "Incomplete trigger at final flush");
    for (bool wlc : {false, true}) {
        auto self = c;
        self.event_builder.event_source.trigger_type = wlc ? "ext" : "self";
        self.event_builder.event_source.wlc_mode = wlc;
        Collector collector(self);
        collector.get_event_builder().collect_events(group(0, 100));
        collector.flush();
        auto out = collector.take_data().second;
        require(out.size() == 1 && out[0]->header.num_packets == 2,
                "Valid time-based event lost at flush");
    }
    auto batch = c;
    batch.event_builder.settings.collection_strategy = "time_batch";
    batch.event_builder.settings.collection_strategies.time_batch.trigger_events
        .event_completion_time_us = 1000000;
    Collector incomplete_batch(batch);
    incomplete_batch.get_event_builder().collect_events(group(0, 100));
    rejects([&] { incomplete_batch.flush(); },
            "Incomplete trigger at final flush");
    Collector tail(c);
    UdpSimulator sim(UdpSimulatorConfig{}, "127.0.0.1", 1, {0}, 2);
    auto raw = sim.trigger_bytes(0);
    tail.get_receiver().getDataBuffer().append(raw.data(), 37);
    tail.collect();
    rejects([&] { tail.flush(); }, "Incomplete raw packet at final flush");
    c.event_builder.event_source.fail_on_data_loss = false;
    Collector loose(c);
    loose.get_event_builder().collect_events(group(0, 100));
    loose.get_event_builder().collect_events(group(0, 200));
    loose.get_event_builder().collect_events(group(3, 200));
    loose.flush();
    auto out = loose.take_data().second;
    require(out.size() == 1 && out[0]->header.reference_time == 200,
            "Final flush blocked behind incomplete event");
    require(loose.take_data().second.empty(), "Finalized event returned twice");
}
void threaded_failure() {
    auto c = config();
    c.udp_receiver.port = free_port();
    c.udp_receiver.max_packet_size = 32;
    Collector collector(c);
    collector.start();
    UdpSimulatorConfig sc;
    sc.triggers_per_second = 100;
    UdpSimulator sim(sc, "127.0.0.1", c.udp_receiver.port, {0}, 2);
    sim.start();
    bool failed = false;
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (std::chrono::steady_clock::now() < deadline) {
        try {
            collector.check_health();
        } catch (const std::exception &) {
            failed = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    sim.stop();
    require(failed, "Worker failure not visible");
    rejects([&] { collector.take_data(); }, "UDP datagram truncated");
    rejects([&] { collector.stop(); }, "UDP datagram truncated");
    collector.stop(); // Cleanup remains safe after the worker already exited.
    auto good = config();
    good.udp_receiver.port = free_port();
    good.event_builder.event_source.trigger_type = "self";
    Collector last(good);
    UdpSimulator shape(UdpSimulatorConfig{}, "127.0.0.1", 1, {0}, 2);
    auto raw = shape.trigger_bytes(0);
    last.get_receiver().getDataBuffer().append(raw.data(), raw.size());
    last.start();
    rejects([&] { last.flush(); }, "Stop background collection");
    last.stop();
    require(last.take_data().second.size() == 1,
            "Threaded stop did not drain/finalize last event");
}
void worker_exception_and_destructor() {
    auto c = config();
    c.event_builder.event_source.channels = {3};
    c.udp_receiver.port = free_port();
    // Receiver stays healthy: this failure is generated by collection itself.
    Collector worker(c);
    UdpSimulator sim(UdpSimulatorConfig{}, "127.0.0.1", 1, {0}, 2);
    const auto raw = sim.trigger_bytes(0);
    worker.get_receiver().getDataBuffer().append(raw.data(), raw.size());
    worker.start();
    bool failed = false;
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (std::chrono::steady_clock::now() < deadline) {
        try {
            worker.check_health();
        } catch (const std::exception &e) {
            require(std::string(e.what()).find("unconfigured channel") !=
                        std::string::npos,
                    e.what());
            failed = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    require(failed, "Collection exception was not propagated from the worker");
    rejects([&] { worker.take_data(); }, "unconfigured channel");
    // Leaving scope must join the failed worker and must not throw/abort.
}

int main() {
    try {
        firmware_957();
        final_flush();
        threaded_failure();
        worker_exception_and_destructor();
        std::cout << "957, physical duplicates, final flush, raw tails, worker "
                     "errors and stop passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
