/**
 * @file nalu_packet_parser.h
 * @brief This file defines the NaluPacketParser class responsible for parsing Nalu packets from a byte stream.
 * 
 * The NaluPacketParser class is used to process raw byte streams, identify valid Nalu packets, and store them in a 
 * vector of NaluPacket structures. The class includes functionality to validate packet integrity with start and stop markers,
 * extract relevant data from the byte stream, and handle any leftover or incomplete packets.
 * 
 * The class is configurable with several parameters such as packet size, marker values, and timing masks.
 * 
 * @note The parsing process can be customized by setting flags for checking packet integrity.
 */

 #ifndef NALU_PACKET_PARSER_H
 #define NALU_PACKET_PARSER_H
 
 #include <vector>
 #include <cstdint>
 #include <iostream>
 #include "nalu_packet.h"  // Including the NaluPacket struct
 #include "nalu_event_collector_params.h"
 
 /**
  * @class NaluPacketParser
  * @brief A class for parsing Nalu packets from a byte stream.
  * 
  * The NaluPacketParser processes byte streams, identifies Nalu packets based on specified start and stop markers, 
  * and extracts relevant data into NaluPacket structures. It supports both marker-based integrity checks and raw parsing
  * for custom use cases.
  */
 class NaluPacketParser {
 public:
     /**
      * @brief Constructor with default values, including header and footer.
      * 
      * @param packet_size The size of each packet in bytes.
      * @param start_marker The start marker used to identify the beginning of a packet.
      * @param stop_marker The stop marker used to identify the end of a packet.
      * @param chan_mask Mask to extract channel data from the byte stream.
      * @param chan_shift Bitwise shift for channel data extraction.
      * @param abs_wind_mask Mask for absolute window data.
      * @param evt_wind_mask Mask for event window data.
      * @param evt_wind_shift Bitwise shift for event window data.
      * @param timing_mask Mask to extract timing data.
      * @param timing_shift Bitwise shift for timing data extraction.
      * @param check_packet_integrity Flag to enable or disable packet integrity checks.
      * @param constructed_packet_header The header to be used for constructed packets.
      * @param constructed_packet_footer The footer to be used for constructed packets.
      */
     NaluPacketParser(size_t packet_size = 74, 
                      const std::vector<uint8_t>& start_marker = {0x0E}, 
                      const std::vector<uint8_t>& stop_marker = {0xFA, 0x5A},
                      uint8_t chan_mask = 0x3F, uint8_t chan_shift = 0,
                      uint8_t abs_wind_mask = 0x3F, uint8_t evt_wind_mask = 0x3F,
                      uint8_t evt_wind_shift = 6, uint16_t timing_mask = 0xFFF,
                      uint8_t timing_shift = 12,
                      bool check_packet_integrity = false,
                      uint16_t constructed_packet_header = 0xAAAA, uint16_t constructed_packet_footer = 0xFFFF);
     
     /**
      * @brief Constructor that accepts parameters encapsulated in a parameter struct.
      * 
      * @param params The NaluPacketParserParams struct containing the configuration values.
      */
     NaluPacketParser(const NaluPacketParserParams& params);
 
     // Getters and setters for the configuration parameters
     size_t get_packet_size() const;
     void set_packet_size(size_t packet_size);
 
     uint8_t get_chan_mask() const;
     void set_chan_mask(uint8_t chan_mask);
 
     uint8_t get_chan_shift() const;
     void set_chan_shift(uint8_t chan_shift);
 
     uint8_t get_abs_wind_mask() const;
     void set_abs_wind_mask(uint8_t abs_wind_mask);
 
     uint8_t get_evt_wind_mask() const;
     void set_evt_wind_mask(uint8_t evt_wind_mask);
 
     uint8_t get_evt_wind_shift() const;
     void set_evt_wind_shift(uint8_t evt_wind_shift);
 
     uint16_t get_timing_mask() const;
     void set_timing_mask(uint16_t timing_mask);
 
     uint8_t get_timing_shift() const;
     void set_timing_shift(uint8_t timing_shift);
 
     std::vector<uint8_t> get_start_marker() const;
     void set_start_marker(const std::vector<uint8_t>& start_marker);
 
     std::vector<uint8_t> get_stop_marker() const;
     void set_stop_marker(const std::vector<uint8_t>& stop_marker);
 
     /**
      * @brief Processes a byte stream and parses Nalu packets.
      * 
      * This method splits the byte stream into Nalu packets, validates each packet, 
      * and returns a vector of parsed NaluPacket structures.
      * 
      * @param byte_stream The raw byte stream to be processed.
      * @return A vector of NaluPacket structures.
      */
     std::vector<NaluPacket> process_stream(const std::vector<uint8_t>& byte_stream);
 
 private:
     size_t packet_size;
     uint8_t chan_mask;
     uint8_t chan_shift;
     uint8_t abs_wind_mask;
     uint8_t evt_wind_mask;
     uint8_t evt_wind_shift;
     uint16_t timing_mask;
     uint8_t timing_shift;
     bool check_packet_integrity; 
     std::vector<uint8_t> start_marker;
     std::vector<uint8_t> stop_marker;
 
     uint16_t packet_index = 0;
     std::vector<uint8_t> leftovers; // Store unprocessed data
     uint16_t constructed_packet_header;
     uint16_t constructed_packet_footer;
 
     /**
      * @brief Processes a single packet from the byte stream.
      * 
      * This method extracts the relevant data from the byte stream and stores it in a NaluPacket structure.
      * 
      * @param packets The vector of NaluPacket structures to store the parsed packet.
      * @param byte_stream The raw byte stream containing the packet data.
      * @param start_index The starting index within the byte stream to begin parsing.
      * @param error_code The error code to associate with the packet.
      */
     void process_packet(std::vector<NaluPacket>& packets, 
                         const uint8_t* byte_stream, size_t start_index,
                         uint8_t error_code);
     
     /**
      * @brief Checks if a specific marker is found at the given position in the byte stream.
      * 
      * @param byte_stream The raw byte stream.
      * @param index The index to start checking from.
      * @param marker The marker to check for.
      * @param marker_len The length of the marker.
      * @return True if the marker is found, otherwise false.
      */
     bool check_marker(const uint8_t* byte_stream, size_t index, const uint8_t* marker, size_t marker_len);
 
     /**
      * @brief Processes the byte stream segment with marker integrity checks.
      * 
      * This method ensures that both the start and stop markers are correctly located in the byte stream segment.
      * 
      * @param packets The vector of NaluPacket structures to store the parsed packets.
      * @param byte_stream The raw byte stream.
      * @param i The current position within the byte stream.
      * @param error_code The error code associated with packet integrity.
      * @param start_marker_len The length of the start marker.
      * @param stop_marker_len The length of the stop marker.
      */
     void process_byte_stream_segment_with_checks(std::vector<NaluPacket>& packets,
                                                 const uint8_t* byte_stream,
                                                 size_t& i, 
                                                 uint8_t& error_code,
                                                 size_t start_marker_len,
                                                 size_t stop_marker_len);
 
     /**
      * @brief Processes the byte stream segment without marker integrity checks.
      * 
      * This method processes the byte stream without validating the markers, used for raw data parsing.
      * 
      * @param packets The vector of NaluPacket structures to store the parsed packets.
      * @param byte_stream The raw byte stream.
      * @param i The current position within the byte stream.
      * @param error_code The error code associated with packet integrity (not used).
      * @param start_marker_len The length of the start marker (not used).
      * @param stop_marker_len The length of the stop marker (not used).
      */
     void process_byte_stream_segment_without_checks(std::vector<NaluPacket>& packets,
                                                     const uint8_t* byte_stream,
                                                     size_t& i, 
                                                     uint8_t& error_code,
                                                     size_t start_marker_len,
                                                     size_t stop_marker_len);
 
     /**
      * @brief Processes any leftovers from previous byte stream processing.
      * 
      * This method ensures that any incomplete packet data from the leftovers buffer is handled properly.
      * 
      * @param data_list The vector to store the parsed data.
      * @param byte_stream The raw byte stream.
      * @param byte_stream_len The length of the byte stream.
      * @param leftovers_size The size of the leftovers buffer.
      * @param packet_size The size of each packet.
      * @param start_marker_len The length of the start marker.
      * @param stop_marker_len The length of the stop marker.
      */
     void process_leftovers(std::vector<NaluPacket>& data_list, 
                         const uint8_t* byte_stream, 
                         size_t byte_stream_len,
                         size_t leftovers_size, 
                         const size_t packet_size,
                         const size_t start_marker_len,
                         const size_t stop_marker_len);
 };
 
 #endif // NALU_PACKET_PARSER_H
 