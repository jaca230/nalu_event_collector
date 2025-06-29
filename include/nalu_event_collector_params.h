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
    std::vector<int> channels = 
        {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31};    ///< Default list of channels.
    std::string trigger_type = "self";  ///< Default trigger type (self, ext, or imm).
    int windows = 4;                  ///< Default number of windows.
    uint32_t time_threshold = 5000;        ///< Default time threshold for event triggering.
    size_t max_events_in_buffer = 10000;  ///< Default maximum number of events in the buffer.
    uint32_t max_trigger_time = 16777216;   ///< Default maximum trigger time in clock cycles.
    uint32_t clock_frequency = 23843000;  ///< Default clock frequency in Hz.
    size_t max_lookback = 2;          ///< Default maximum lookback period for events.
    uint16_t event_header = 0xBBBB;        ///< Default header value for event.
    uint16_t event_trailer = 0xEEEE;       ///< Default trailer value for event.
};

/**
 * @brief Struct for NaluUdpReceiver parameters.
 *
 * This struct holds the configuration parameters for the NaluUdpReceiver, which
 * handles the UDP communication and receives raw packet data.
 */
struct NaluUdpReceiverParams {
    std::string address = "127.0.0.1";     ///< Default IP address for UDP receiver.
    uint16_t port = 9000;           ///< Default UDP port for receiver.
    size_t buffer_size = 1024 * 1024 * 100;      ///< Default size of the buffer for storing incoming data.
    size_t max_packet_size = 1040;  ///< Default maximum size of a single packet.
    int timeout_sec = 10;  ///< Default timeout duration in seconds for receiving data.
};

/**
 * @brief Struct for NaluPacketParser parameters.
 *
 * This struct holds the configuration parameters for the NaluPacketParser,
 * which parses incoming NALU packets and extracts relevant information.
 */
struct NaluPacketParserParams {
    size_t packet_size = 74;  ///< Default size of a single packet.
    std::string start_marker = "0E";  ///< Default start marker in hex string format.
    std::string stop_marker = "FA5A";  ///< Default stop marker in hex string format.
    uint8_t chan_mask = 0x3F;       ///< Default channel mask for packet extraction.
    uint8_t chan_shift = 0;      ///< Default shift value for channel extraction.
    uint8_t abs_wind_mask = 0x3F;   ///< Default absolute window mask for packet processing.
    uint8_t evt_wind_mask = 0x3F;   ///< Default event window mask for packet processing.
    uint8_t evt_wind_shift = 6;  ///< Default event window shift value.
    uint16_t timing_mask = 0xFFF;  ///< Default mask for timing-related information in the packet.
    uint8_t timing_shift = 12;         ///< Default shift for timing-related information.
    bool check_packet_integrity = false;  ///< Default flag indicating whether to check packet integrity.
    uint16_t constructed_packet_header = 0xAAAA;  ///< Default header value for the constructed packet.
    uint16_t constructed_packet_footer = 0xFFFF;  ///< Default footer value for the constructed packet.
};

/**
 * @brief Struct that holds all parameters required for the NaluEventCollector.
 *
 * This struct aggregates all the configuration parameters for the
 * NaluEventCollector class, including the NaluEventBuilder, NaluUdpReceiver,
 * and NaluPacketParser parameters.
 */
struct NaluEventCollectorParams {
    NaluEventBuilderParams event_builder_params;  ///< Parameters for the NaluEventBuilder.
    NaluUdpReceiverParams udp_receiver_params;  ///< Parameters for the NaluUdpReceiver.
    NaluPacketParserParams packet_parser_params;  ///< Parameters for the NaluPacketParser.
    std::chrono::microseconds sleep_time_us = std::chrono::microseconds(-1);  ///< Default sleep time between collection cycles.
};

#endif  // NALU_EVENT_COLLECTOR_PARAMS_H
