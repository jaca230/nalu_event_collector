#include "nalu_event_collector/simulation/udp_simulator.h"
#include <algorithm>
#include <arpa/inet.h>
#include <chrono>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <unistd.h>
namespace nalu_event_collector {
UdpSimulator::UdpSimulator(UdpSimulatorConfig c, std::string a, uint16_t p,
                           std::vector<int> channels, unsigned windows)
    : config_(c), address_(std::move(a)), port_(p),
      channels_(std::move(channels)), windows_(windows) {
  if (!std::isfinite(c.triggers_per_second) || c.triggers_per_second <= 0 ||
      c.triggers_per_second > c.clock_frequency || !c.clock_frequency ||
      c.timestamp_modulus < 2 || c.timestamp_modulus > (1U << 28) ||
      c.payload_bytes == 0 || c.payload_bytes > 65000 || windows == 0 ||
      windows > 62 || channels_.empty())
    throw std::invalid_argument("Invalid UDP simulator configuration");
  for (int channel : channels_)
    if (channel < 0 || channel >= 32)
      throw std::invalid_argument("Simulator channel outside [0,31]");
  // Precompute the shaped pulses; no exponentials in the UDP sending loop.
  const double rise = std::min(6.0, double(windows_));
  const double fall = 6 * rise;
  const double peak_time = std::log(fall / rise) / (1 / rise - 1 / fall);
  const double peak = std::exp(-peak_time / fall) - std::exp(-peak_time / rise);
  for (int channel : channels_) {
    auto& trace = waveform_adc_[channel];
    trace.resize(windows_ * 32);
    const int onset = int(windows_ * 16) - std::min(24, int(windows_ * 4)) + channel % 9 - 4;
    for (unsigned sample = 0; sample < trace.size(); ++sample) {
      const double t = double(sample) - onset;
      const double pulse = t > 0 ? (std::exp(-t / fall) - std::exp(-t / rise)) / peak : 0;
      trace[sample] = uint16_t(1800 + channel + std::lround(480 * pulse));
    }
  }

}
UdpSimulator::~UdpSimulator() { stop(); }
std::vector<uint8_t> UdpSimulator::trigger_bytes(uint64_t index) const {
  std::vector<uint8_t> bytes;
  bytes.reserve(channels_.size() * windows_ * 74);
  const uint32_t tick = static_cast<uint32_t>(
      std::fmod(config_.first_timestamp + static_cast<long double>(index) *
                                              config_.clock_frequency /
                                              config_.triggers_per_second,
                config_.timestamp_modulus));
  for (int channel : channels_)
    for (unsigned window = 0; window < windows_; ++window) {
      const auto offset = bytes.size();
      bytes.resize(offset + 74);
      auto *p = bytes.data() + offset;
      p[0] = 0x0e;
      p[1] = channel;
      p[2] = tick >> 20;
      p[3] = tick >> 12;
      p[4] = (tick >> 8) & 15;
      p[5] = tick;
      p[6] = (window >> 2) & 63;
      p[7] = ((window & 3) << 6) | (window & 63);
      for (unsigned sample = 0; sample < 32; ++sample) {
        const uint16_t adc = waveform_adc_[channel][window * 32 + sample];
        p[8 + 2 * sample] = adc >> 8;
        p[9 + 2 * sample] = adc;
      }
      p[72] = 0xfa;
      p[73] = 0x5a;
    }
  return bytes;
}
void UdpSimulator::check_health() const {
  std::lock_guard lock(error_mutex_);
  if (!error_.empty())
    throw std::runtime_error(error_);
}
void UdpSimulator::start() {
  if (running_)
    return;
  check_health();
  sent_ = 0;
  running_ = true;
  thread_ = std::thread(&UdpSimulator::run, this);
}
void UdpSimulator::stop() {
  running_ = false;
  if (thread_.joinable())
    thread_.join();
}
void UdpSimulator::run() {
  const int fd = socket(AF_INET, SOCK_DGRAM, 0);
  try {
    if (fd < 0)
      throw std::runtime_error("Simulator socket failed");
    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_port = htons(port_);
    if (inet_pton(AF_INET, address_.c_str(), &dest.sin_addr) != 1)
      throw std::runtime_error("Invalid simulator IPv4 destination");
    const auto start = std::chrono::steady_clock::now();
    uint64_t index = 0;
    while (running_) {
      auto deadline = start + std::chrono::nanoseconds(static_cast<int64_t>(
                                  index * 1e9 / config_.triggers_per_second));
      // Bounded waits allow prompt run stop at low requested rates.
      while (running_ && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_until(
            std::min(deadline, std::chrono::steady_clock::now() +
                                   std::chrono::milliseconds(1)));
      if (!running_)
        break;
      const auto stream = trigger_bytes(index);
      for (size_t offset = 0; offset < stream.size();
           offset += config_.payload_bytes) {
        const auto count =
            std::min<size_t>(config_.payload_bytes, stream.size() - offset);
        std::vector<uint8_t> packet(16 + count, 0);
        const uint16_t size = htons(count);
        std::memcpy(packet.data(), &size, 2);
        std::memcpy(packet.data() + 16, stream.data() + offset, count);
        if (sendto(fd, packet.data(), packet.size(), 0,
                   reinterpret_cast<sockaddr *>(&dest),
                   sizeof(dest)) != static_cast<ssize_t>(packet.size()))
          throw std::runtime_error("Simulator UDP send failed");
      }
      ++index;
      sent_ = index;
    }
  } catch (const std::exception &e) {
    std::lock_guard lock(error_mutex_);
    error_ = e.what();
    running_ = false;
  }
  if (fd >= 0)
    close(fd);
}
} // namespace nalu_event_collector
