#include "nalu_event_collector/network/udp_receiver.h"
#include "nalu_event_collector/collector/collector.h"
#include "nalu_event_collector/parsing/packet_parser.h"
#include "nalu_event_collector/simulation/udp_simulator.h"
#include <chrono>
#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <thread>
using namespace nalu_event_collector;
void require(bool b) {
  if (!b)
    throw std::runtime_error("simulator assertion failed");
}
int main() {
  UdpSimulatorConfig c;
  c.first_timestamp = 16777215;
  c.triggers_per_second = 23843000;
  UdpSimulator simulator(c, "127.0.0.1", 19100, {0, 3}, 2);
  PacketParser parser(PacketParserConfig{});
  auto p = parser.process_stream(simulator.trigger_bytes(1));
  require(p.size() == 4 && p[0].trigger_time == 0 && p[3].channel == 3 &&
          p[3].logical_position == 1);
  // Pulse shape is continuous across packets, including maximum readout sizes.
  for (unsigned windows : {1U, 2U, 8U, 61U, 62U}) {
    UdpSimulator shape(c, "127.0.0.1", 19102, {0, 3}, windows);
    auto packets = parser.process_stream(shape.trigger_bytes(0));
    require(packets.size() == 2 * windows);
    for (unsigned chidx = 0; chidx < 2; ++chidx) {
      const unsigned channel = chidx == 0 ? 0 : 3;
      const int onset = windows*16 - std::min(24U,windows*4) + channel%9 - 4;
      const double rise = std::min(6.0,double(windows)), fall = 6*rise;
      const double peak_t = std::log(6.0)/(1/rise-1/fall);
      const double norm = std::exp(-peak_t/fall)-std::exp(-peak_t/rise);
      unsigned maximum = 0;
      for (unsigned w = 0; w < windows; ++w)
        for (unsigned k = 0; k < 32; ++k) {
          const auto& packet = packets[chidx * windows + w];
          const auto adc = unsigned(packet.raw_samples[2*k]) * 256 + packet.raw_samples[2*k+1];
          const double t = double(w*32+k)-onset;
          const double pulse = t>0 ? (std::exp(-t/fall)-std::exp(-t/rise))/norm : 0;
          require(adc == 1800 + channel + std::lround(480*pulse));
          maximum = std::max(maximum, adc);
        }
      require(maximum >= 1800+channel+478 && maximum <= 1800+channel+480);
    }
  }
  UdpReceiverConfig config;
  config.port = 19100;
  config.max_packet_size = 1040;
  UdpReceiver receiver(config);
  receiver.start();
  c.triggers_per_second=1000;
  UdpSimulator live(c,"127.0.0.1",19100,{0,3},2);
  live.start();
  std::this_thread::sleep_for(std::chrono::milliseconds(2));
  live.stop();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  receiver.stop();
  receiver.check_health();
  require(receiver.getDataBuffer().size() > 0);
  UdpReceiverConfig small;
  small.port = 19101;
  small.max_packet_size = 32;
  UdpReceiver truncated(small);
  truncated.start();
  c.triggers_per_second = 100;
  UdpSimulator sender(c, "127.0.0.1", 19101, {0}, 2);
  sender.start();
  std::this_thread::sleep_for(std::chrono::milliseconds(30));
  sender.stop();
  truncated.stop();
  bool threw = false;
  try {
    truncated.check_health();
  } catch (const std::exception &) {
    threw = true;
  }
  require(threw);
  CollectorConfig strict;
  strict.event_builder.event_source.channels={0};
  strict.event_builder.event_source.windows=2;
  strict.event_builder.event_source.trigger_type="ext";
  strict.event_builder.event_source.fail_on_data_loss=true;
  strict.event_builder.settings.collection_strategies.trigger_time.event_completion_time_us=1000;
  Collector collector(strict);
  auto incomplete=sender.trigger_bytes(0);
  collector.get_receiver().getDataBuffer().append(incomplete.data(),74);
  collector.collect();
  std::this_thread::sleep_for(std::chrono::milliseconds(3));
  threw=false;try{collector.collect();}catch(const std::exception&){threw=true;}require(threw);
  std::cout << "UDP wire parser, timestamp wrap, live socket and fatal "
               "truncation passed\n";
}
