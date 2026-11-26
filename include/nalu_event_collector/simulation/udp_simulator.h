#pragma once
#include <atomic>
#include <array>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace nalu_event_collector {
struct UdpSimulatorConfig {
  bool enabled = false;
  double triggers_per_second = 1000;
  uint32_t clock_frequency = 23843000;
  uint32_t timestamp_modulus = 16777216;
  uint32_t first_timestamp = 1000;
  uint16_t payload_bytes = 1024;
};
// Generates the actual 16-byte UDP transport header and 74-byte window stream.
// The collector's normal receiver, parser and channel grouping remain in use.
class UdpSimulator {
public:
  UdpSimulator(UdpSimulatorConfig config, std::string address, uint16_t port,
               std::vector<int> channels, unsigned windows);
  ~UdpSimulator();
  std::vector<uint8_t> trigger_bytes(uint64_t index) const;
  void start();
  void stop();
  void check_health() const;
  uint64_t sent_triggers() const { return sent_; }

private:
  void run();
  UdpSimulatorConfig config_;
  std::string address_;
  uint16_t port_;
  std::vector<int> channels_;
  unsigned windows_;
  std::array<std::vector<uint16_t>, 32> waveform_adc_;
  std::atomic<bool> running_{false};
  std::atomic<uint64_t> sent_{0};
  std::thread thread_;
  mutable std::mutex error_mutex_;
  std::string error_;
};
} // namespace nalu_event_collector
