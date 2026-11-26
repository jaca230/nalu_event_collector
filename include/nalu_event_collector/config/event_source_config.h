/**
 * @file event_source_config.h
 * @brief Acquisition context shared by packet collection strategies.
 */

#pragma once

#include <string>
#include <vector>

namespace nalu_event_collector {

inline std::vector<int> default_channels() {
    std::vector<int> channels;
    channels.reserve(32);
    for (int channel = 0; channel < 32; ++channel) {
        channels.push_back(channel);
    }
    return channels;
}

struct EventSourceConfig {
    std::vector<int> channels = default_channels();
    std::string trigger_type = "self";
    int windows = 4;
    bool wlc_mode = false;
    bool fail_on_data_loss = false;
};

}  // namespace nalu_event_collector
