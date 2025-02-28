#ifndef NALU_EVENT_COLLECTOR_PARAMS_H
#define NALU_EVENT_COLLECTOR_PARAMS_H

#include <vector>
#include <string>
#include <chrono>

// Struct for NaluEventBuilder parameters
struct NaluEventBuilderParams {
    std::vector<int> channels;
    int windows;
    int time_threshold;
    size_t max_events_in_buffer;
    uint32_t max_trigger_time;
    size_t max_lookback;
    uint16_t event_header;
    uint16_t event_trailer;

    // Default constructor with default values
    NaluEventBuilderParams(std::vector<int> ch = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
                           int win = 4, int time_thresh = 5000, size_t max_buf = 1000000, uint32_t max_time = 16777216,
                           size_t max_look = 2, uint16_t header = 0xBBBB, uint16_t trailer = 0xEEEE)
        : channels(ch), windows(win), time_threshold(time_thresh), max_events_in_buffer(max_buf),
          max_trigger_time(max_time), max_lookback(max_look), event_header(header), event_trailer(trailer) {}
};

// Struct for NaluUdpReceiver parameters
struct NaluUdpReceiverParams {
    std::string address;
    uint16_t port;
    size_t buffer_size;
    size_t max_packet_size;
    int timeout_sec;

    // Default constructor with default values
    NaluUdpReceiverParams(std::string addr = "127.0.0.1", uint16_t p = 9000,
                          size_t buf_size = 1024 * 1024 * 100, size_t pkt_size = 1040, int timeout = 10)
        : address(addr), port(p), buffer_size(buf_size), max_packet_size(pkt_size), timeout_sec(timeout) {}
};

// Struct for NaluPacketParser parameters
struct NaluPacketParserParams {
    size_t packet_size;
    std::vector<uint8_t> start_marker;
    std::vector<uint8_t> stop_marker;
    uint8_t chan_mask;
    uint8_t chan_shift;
    uint8_t abs_wind_mask;
    uint8_t evt_wind_mask;
    uint8_t evt_wind_shift;
    uint16_t timing_mask;
    uint8_t timing_shift;
    bool check_packet_integrity;
    uint16_t constructed_packet_header;
    uint16_t constructed_packet_footer;

    // Default constructor with default values
    NaluPacketParserParams(size_t pkt_size = 74,
                           std::vector<uint8_t> start_m = {0x0E}, std::vector<uint8_t> stop_m = {0xFA, 0x5A},
                           uint8_t ch_mask = 0x3F, uint8_t ch_shift = 0, uint8_t abs_w_mask = 0x3F,
                           uint8_t evt_w_mask = 0x3F, uint8_t evt_w_shift = 6, uint16_t t_mask = 0xFFF,
                           uint8_t t_shift = 12, bool integrity = false, uint16_t header = 0xAAAA, uint16_t footer = 0xFFFF)
        : packet_size(pkt_size), start_marker(start_m), stop_marker(stop_m), chan_mask(ch_mask),
          chan_shift(ch_shift), abs_wind_mask(abs_w_mask), evt_wind_mask(evt_w_mask), evt_wind_shift(evt_w_shift),
          timing_mask(t_mask), timing_shift(t_shift), check_packet_integrity(integrity),
          constructed_packet_header(header), constructed_packet_footer(footer) {}
};

// A single struct to hold all the parameters
struct NaluEventCollectorParams {
    NaluEventBuilderParams event_builder_params;
    NaluUdpReceiverParams udp_receiver_params;
    NaluPacketParserParams packet_parser_params;
    std::chrono::microseconds sleep_time_us;  // Changed to sleep_time

    // Default constructor
    NaluEventCollectorParams(
        NaluEventBuilderParams eb_params = NaluEventBuilderParams(),
        NaluUdpReceiverParams ur_params = NaluUdpReceiverParams(),
        NaluPacketParserParams pp_params = NaluPacketParserParams(),
        std::chrono::microseconds sleep_time_us = std::chrono::microseconds(-1)
    )
        : event_builder_params(eb_params),
          udp_receiver_params(ur_params),
          packet_parser_params(pp_params),
          sleep_time_us(sleep_time_us) {}
};

#endif // NALU_EVENT_COLLECTOR_PARAMS_H
