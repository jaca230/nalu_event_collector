/**
 * @file event.cpp
 * @brief Implements the serialized event data model and completeness logic.
 */

#include "nalu_event_collector/data/event.h"

#include <cstring>
#include <iostream>
#include <stdexcept>

#include <spdlog/spdlog.h>

namespace nalu_event_collector {

Event::Event()
    : header{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      footer{0},
      max_packets(0),
      packets(nullptr),
      use_time_based_completion_(false),
      creation_timestamp(std::chrono::steady_clock::now()) {}

Event::Event(uint16_t hdr,
             uint8_t extra_info,
             uint32_t idx,
             uint32_t ref_time,
             uint32_t time_thresh,
             uint32_t clk_freq,
             uint32_t event_completion_time_us,
             uint16_t size,
             uint16_t num,
             uint16_t ftr,
             uint16_t max_num_packets,
             uint64_t channel_mask_value,
             uint8_t num_windows_value,
             bool use_time_based_completion,
             uint16_t expected_packet_count,
             bool warn_on_expected_overrun)
    : header{hdr,
             extra_info,
             idx,
             ref_time,
             time_thresh,
             event_completion_time_us,
             clk_freq,
             size,
             channel_mask_value,
             num_windows_value,
             num},
      packets(std::make_unique<Packet[]>(max_num_packets)),
      footer{ftr},
      max_packets(max_num_packets),
      use_time_based_completion_(use_time_based_completion),
      expected_packet_count_(expected_packet_count),
      warn_on_expected_overrun_(warn_on_expected_overrun),
      creation_timestamp(std::chrono::steady_clock::now()) {}

void Event::print_event_info() const {
    std::cout << "Event Header:\n";
    std::cout << "Header: " << header.header << '\n';
    std::cout << "Info: " << static_cast<int>(header.info) << '\n';
    std::cout << "Index: " << header.index << '\n';
    std::cout << "Reference Time: " << header.reference_time << '\n';
    std::cout << "Time Threshold (ticks): " << header.time_threshold << '\n';
    std::cout << "Event Completion Time (us): " << header.event_completion_time_us << '\n';
    std::cout << "Clock Frequency (Hz): " << header.clock_frequency << '\n';
    std::cout << "Packet Size: " << header.packet_size << '\n';
    std::cout << "Channel Mask: " << header.channel_mask << '\n';
    std::cout << "Number of Windows: " << static_cast<int>(header.num_windows) << '\n';
    std::cout << "Number of Packets: " << header.num_packets << '\n';
    std::cout << "Event Footer: " << footer.footer << '\n';
}

uint8_t Event::get_error_code() const { return header.info & 0x0F; }

Event::TriggerType Event::get_trigger_type() const {
    const uint8_t trigger_bits = (header.info & 0x30) >> 4;
    switch (trigger_bits) {
        case 1:
            return TriggerType::External;
        case 2:
            return TriggerType::Internal;
        case 3:
            return TriggerType::Immediate;
        default:
            return TriggerType::Unknown;
    }
}

size_t Event::get_size() const {
    return sizeof(header) + (header.num_packets * sizeof(Packet)) + sizeof(footer);
}

void Event::serialize_to_buffer(char* buffer) const {
    if (buffer == nullptr) {
        return;
    }

    size_t offset = 0;
    std::memcpy(buffer + offset, &header, sizeof(header));
    offset += sizeof(header);
    std::memcpy(buffer + offset, packets.get(), header.num_packets * sizeof(Packet));
    offset += header.num_packets * sizeof(Packet);
    std::memcpy(buffer + offset, &footer, sizeof(footer));
}

void Event::add_packet(const Packet& packet) {
    if (header.num_packets >= max_packets) {
        // This will basically never fire since we set max packets to a numeric limit
        // Kept here as legacy/defensive code in case of future changes to max_packets handling
        spdlog::error("Attempt to add packet exceeds max packet limit. max={}, current={}",
                      max_packets,
                      header.num_packets);
        throw std::overflow_error("Maximum number of packets exceeded.");
    }

    packets[header.num_packets] = packet;
    ++header.num_packets;
    if (packet.channel < channel_packet_counts_.size()) {
        ++channel_packet_counts_[packet.channel];
    }

    if (warn_on_expected_overrun_ && !warned_on_expected_overrun_ &&
        expected_packet_count_ > 0 && header.num_packets > expected_packet_count_) {
        warned_on_expected_overrun_ = true;
        spdlog::warn(
            "External-trigger event index {} exceeded expected packet count: expected={}, actual={}, "
            "reference_time={}, channel_mask=0x{:x}, windows={}",
            header.index,
            expected_packet_count_,
            header.num_packets,
            header.reference_time,
            header.channel_mask,
            header.num_windows);
    }
}

bool Event::can_accept_channel_group(uint8_t channel, size_t packet_count) const {
    if (channel >= channel_packet_counts_.size() || packet_count != header.num_windows ||
        header.num_windows == 0 || is_event_complete()) {
        return false;
    }

    const uint64_t channel_bit = 1ULL << channel;
    return (header.channel_mask & channel_bit) != 0 && channel_packet_counts_[channel] == 0;
}

void Event::add_packet_group(const std::vector<Packet>& packet_group) {
    if (packet_group.empty()) {
        throw std::invalid_argument("Cannot add an empty channel packet group.");
    }

    const uint8_t channel = packet_group.front().channel;
    const uint32_t trigger_time = packet_group.front().trigger_time;
    if (!can_accept_channel_group(channel, packet_group.size())) {
        throw std::invalid_argument("Event cannot accept this channel packet group.");
    }

    for (const auto& packet : packet_group) {
        if (packet.channel != channel || packet.trigger_time != trigger_time) {
            throw std::invalid_argument(
                "A channel packet group must have one channel and one trigger timestamp.");
        }
    }

    for (const auto& packet : packet_group) {
        add_packet(packet);
    }
}

uint16_t Event::get_channel_packet_count(uint8_t channel) const {
    if (channel >= channel_packet_counts_.size()) {
        return 0;
    }
    return channel_packet_counts_[channel];
}

int Event::count_active_channels(uint64_t channel_mask) const {
    int count = 0;
    while (channel_mask != 0) {
        count += static_cast<int>(channel_mask & 1U);
        channel_mask >>= 1U;
    }
    return count;
}

bool Event::is_event_complete() const {
    if (use_time_based_completion_) {
        const auto elapsed = std::chrono::steady_clock::now() - creation_timestamp;
        const uint32_t completion_time_us = header.event_completion_time_us;
        return elapsed >= std::chrono::microseconds(completion_time_us);
    }

    const int active_channels = count_active_channels(header.channel_mask);

    switch (get_trigger_type()) {
        case TriggerType::Internal: {
            const auto elapsed = std::chrono::steady_clock::now() - creation_timestamp;
            const uint32_t completion_time_us = header.event_completion_time_us;
            return elapsed >= std::chrono::microseconds(completion_time_us);
        }
        case TriggerType::External:
        case TriggerType::Immediate:
        case TriggerType::Unknown:
        default:
            if (active_channels == 0 || header.num_windows == 0) {
                return false;
            }
            for (size_t channel = 0; channel < channel_packet_counts_.size(); ++channel) {
                if ((header.channel_mask & (1ULL << channel)) != 0 &&
                    channel_packet_counts_[channel] != header.num_windows) {
                    return false;
                }
            }
            return true;
    }
}

bool Event::has_timed_out() const {
    const auto elapsed = std::chrono::steady_clock::now() - creation_timestamp;
    const uint32_t completion_time_us = header.event_completion_time_us;
    return elapsed >= std::chrono::microseconds(completion_time_us);
}

bool Event::is_event_complete(int windows,
                              const std::vector<int>& channels,
                              std::string trigger_type_str,
                              bool use_time_based_completion,
                              std::chrono::steady_clock::duration max_time_between_events) const {
    if (use_time_based_completion) {
        return (std::chrono::steady_clock::now() - creation_timestamp) >= max_time_between_events;
    }

    TriggerType trigger_type = TriggerType::Unknown;
    if (trigger_type_str == "ext") {
        trigger_type = TriggerType::External;
    } else if (trigger_type_str == "self") {
        trigger_type = TriggerType::Internal;
    } else if (trigger_type_str == "imm") {
        trigger_type = TriggerType::Immediate;
    }

    switch (trigger_type) {
        case TriggerType::Internal:
            return (std::chrono::steady_clock::now() - creation_timestamp) >= max_time_between_events;
        case TriggerType::External:
        case TriggerType::Immediate:
        case TriggerType::Unknown:
        default:
            if (windows <= 0 || channels.empty()) {
                return false;
            }
            for (int channel : channels) {
                if (channel < 0 || channel >= static_cast<int>(channel_packet_counts_.size()) ||
                    channel_packet_counts_[static_cast<size_t>(channel)] != windows) {
                    return false;
                }
            }
            return true;
    }
}

std::chrono::steady_clock::time_point Event::get_creation_timestamp() const {
    return creation_timestamp;
}

}  // namespace nalu_event_collector
