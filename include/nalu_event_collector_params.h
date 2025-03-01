#ifndef NALU_EVENT_COLLECTOR_PARAMS_H
#define NALU_EVENT_COLLECTOR_PARAMS_H

#include <chrono>
#include <string>
#include <vector>

/**
 * @brief Struct for NaluEventBuilder parameters.
 *
 * This struct holds the configuration parameters for the NaluEventBuilder,
 * which is responsible for collecting and managing events from NALU packets.
 */
struct NaluEventBuilderParams {
    std::vector<int> channels;    ///< List of channels.
    int windows;                  ///< Number of windows.
    int time_threshold;           ///< Time threshold for event triggering.
    size_t max_events_in_buffer;  ///< Maximum number of events in the buffer.
    uint32_t max_trigger_time;    ///< Maximum trigger time in clock cycles.
    size_t max_lookback;          ///< Maximum lookback period for events.
    uint16_t event_header;        ///< Header value for event.
    uint16_t event_trailer;       ///< Trailer value for event.

    /**
     * @brief Default constructor with default values for
     * NaluEventBuilderParams.
     *
     * Initializes parameters to their default values if not provided.
     */
    NaluEventBuilderParams(std::vector<int> ch = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
                                                  10, 11, 12, 13, 14, 15},
                           int win = 4, int time_thresh = 5000,
                           size_t max_buf = 1000000,
                           uint32_t max_time = 16777216, size_t max_look = 2,
                           uint16_t header = 0xBBBB, uint16_t trailer = 0xEEEE)
        : channels(ch),
          windows(win),
          time_threshold(time_thresh),
          max_events_in_buffer(max_buf),
          max_trigger_time(max_time),
          max_lookback(max_look),
          event_header(header),
          event_trailer(trailer) {}
};

/**
 * @brief Struct for NaluUdpReceiver parameters.
 *
 * This struct holds the configuration parameters for the NaluUdpReceiver, which
 * handles the UDP communication and receives raw packet data.
 */
struct NaluUdpReceiverParams {
    std::string address;     ///< IP address for UDP receiver.
    uint16_t port;           ///< UDP port for receiver.
    size_t buffer_size;      ///< Size of the buffer for storing incoming data.
    size_t max_packet_size;  ///< Maximum size of a single packet.
    int timeout_sec;  ///< Timeout duration in seconds for receiving data.

    /**
     * @brief Default constructor with default values for NaluUdpReceiverParams.
     *
     * Initializes parameters to their default values if not provided.
     */
    NaluUdpReceiverParams(std::string addr = "127.0.0.1", uint16_t p = 9000,
                          size_t buf_size = 1024 * 1024 * 100,
                          size_t pkt_size = 1040, int timeout = 10)
        : address(addr),
          port(p),
          buffer_size(buf_size),
          max_packet_size(pkt_size),
          timeout_sec(timeout) {}
};

/**
 * @brief Struct for NaluPacketParser parameters.
 *
 * This struct holds the configuration parameters for the NaluPacketParser,
 * which parses incoming NALU packets and extracts relevant information.
 */
struct NaluPacketParserParams {
    size_t packet_size;  ///< Size of a single packet.
    std::vector<uint8_t>
        start_marker;  ///< Start marker to identify the beginning of a packet.
    std::vector<uint8_t>
        stop_marker;         ///< Stop marker to identify the end of a packet.
    uint8_t chan_mask;       ///< Channel mask for packet extraction.
    uint8_t chan_shift;      ///< Shift value for channel extraction.
    uint8_t abs_wind_mask;   ///< Absolute window mask for packet processing.
    uint8_t evt_wind_mask;   ///< Event window mask for packet processing.
    uint8_t evt_wind_shift;  ///< Event window shift value.
    uint16_t
        timing_mask;  ///< Mask for timing-related information in the packet.
    uint8_t timing_shift;         ///< Shift for timing-related information.
    bool check_packet_integrity;  ///< Flag indicating whether to check packet
                                  ///< integrity.
    uint16_t constructed_packet_header;  ///< Header value for the constructed
                                         ///< packet.
    uint16_t constructed_packet_footer;  ///< Footer value for the constructed
                                         ///< packet.

    /**
     * @brief Default constructor with default values for
     * NaluPacketParserParams.
     *
     * Initializes parameters to their default values if not provided.
     */
    NaluPacketParserParams(size_t pkt_size = 74,
                           std::vector<uint8_t> start_m = {0x0E},
                           std::vector<uint8_t> stop_m = {0xFA, 0x5A},
                           uint8_t ch_mask = 0x3F, uint8_t ch_shift = 0,
                           uint8_t abs_w_mask = 0x3F, uint8_t evt_w_mask = 0x3F,
                           uint8_t evt_w_shift = 6, uint16_t t_mask = 0xFFF,
                           uint8_t t_shift = 12, bool integrity = false,
                           uint16_t header = 0xAAAA, uint16_t footer = 0xFFFF)
        : packet_size(pkt_size),
          start_marker(start_m),
          stop_marker(stop_m),
          chan_mask(ch_mask),
          chan_shift(ch_shift),
          abs_wind_mask(abs_w_mask),
          evt_wind_mask(evt_w_mask),
          evt_wind_shift(evt_w_shift),
          timing_mask(t_mask),
          timing_shift(t_shift),
          check_packet_integrity(integrity),
          constructed_packet_header(header),
          constructed_packet_footer(footer) {}
};

/**
 * @brief Struct that holds all parameters required for the NaluEventCollector.
 *
 * This struct aggregates all the configuration parameters for the
 * NaluEventCollector class, including the NaluEventBuilder, NaluUdpReceiver,
 * and NaluPacketParser parameters.
 */
struct NaluEventCollectorParams {
    NaluEventBuilderParams
        event_builder_params;  ///< Parameters for the NaluEventBuilder.
    NaluUdpReceiverParams
        udp_receiver_params;  ///< Parameters for the NaluUdpReceiver.
    NaluPacketParserParams
        packet_parser_params;  ///< Parameters for the NaluPacketParser.
    std::chrono::microseconds
        sleep_time_us;  ///< Sleep time between collection cycles.

    /**
     * @brief Default constructor that initializes all parameters.
     *
     * Allows custom initialization for all parameters, with default values
     * provided for each.
     */
    NaluEventCollectorParams(
        NaluEventBuilderParams eb_params = NaluEventBuilderParams(),
        NaluUdpReceiverParams ur_params = NaluUdpReceiverParams(),
        NaluPacketParserParams pp_params = NaluPacketParserParams(),
        std::chrono::microseconds sleep_time_us = std::chrono::microseconds(-1))
        : event_builder_params(eb_params),
          udp_receiver_params(ur_params),
          packet_parser_params(pp_params),
          sleep_time_us(sleep_time_us) {}
};

#endif  // NALU_EVENT_COLLECTOR_PARAMS_H
