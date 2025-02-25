#include "nalu_packet_parser.h"
#include <cstring>
#include <iostream>

// Constructor
NaluPacketParser::NaluPacketParser(size_t packet_size, 
                                   const std::vector<uint8_t>& start_marker, 
                                   const std::vector<uint8_t>& stop_marker,
                                   uint8_t chan_mask, uint8_t chan_shift,
                                   uint8_t abs_wind_mask, uint8_t evt_wind_mask,
                                   uint8_t evt_wind_shift, uint16_t timing_mask,
                                   uint8_t timing_shift)
    : packet_size(packet_size), chan_mask(chan_mask), chan_shift(chan_shift),
      abs_wind_mask(abs_wind_mask), evt_wind_mask(evt_wind_mask),
      evt_wind_shift(evt_wind_shift), timing_mask(timing_mask),
      timing_shift(timing_shift), start_marker(start_marker),
      stop_marker(stop_marker) {}

// Getters and setters

size_t NaluPacketParser::get_packet_size() const {
    return packet_size;
}

void NaluPacketParser::set_packet_size(size_t packet_size) {
    this->packet_size = packet_size;
}

uint8_t NaluPacketParser::get_chan_mask() const {
    return chan_mask;
}

void NaluPacketParser::set_chan_mask(uint8_t chan_mask) {
    this->chan_mask = chan_mask;
}

uint8_t NaluPacketParser::get_chan_shift() const {
    return chan_shift;
}

void NaluPacketParser::set_chan_shift(uint8_t chan_shift) {
    this->chan_shift = chan_shift;
}

uint8_t NaluPacketParser::get_abs_wind_mask() const {
    return abs_wind_mask;
}

void NaluPacketParser::set_abs_wind_mask(uint8_t abs_wind_mask) {
    this->abs_wind_mask = abs_wind_mask;
}

uint8_t NaluPacketParser::get_evt_wind_mask() const {
    return evt_wind_mask;
}

void NaluPacketParser::set_evt_wind_mask(uint8_t evt_wind_mask) {
    this->evt_wind_mask = evt_wind_mask;
}

uint8_t NaluPacketParser::get_evt_wind_shift() const {
    return evt_wind_shift;
}

void NaluPacketParser::set_evt_wind_shift(uint8_t evt_wind_shift) {
    this->evt_wind_shift = evt_wind_shift;
}

uint16_t NaluPacketParser::get_timing_mask() const {
    return timing_mask;
}

void NaluPacketParser::set_timing_mask(uint16_t timing_mask) {
    this->timing_mask = timing_mask;
}

uint8_t NaluPacketParser::get_timing_shift() const {
    return timing_shift;
}

void NaluPacketParser::set_timing_shift(uint8_t timing_shift) {
    this->timing_shift = timing_shift;
}

std::vector<uint8_t> NaluPacketParser::get_start_marker() const {
    return start_marker;
}

void NaluPacketParser::set_start_marker(const std::vector<uint8_t>& start_marker) {
    this->start_marker = start_marker;
}

std::vector<uint8_t> NaluPacketParser::get_stop_marker() const {
    return stop_marker;
}

void NaluPacketParser::set_stop_marker(const std::vector<uint8_t>& stop_marker) {
    this->stop_marker = stop_marker;
}

std::vector<NaluPacket> NaluPacketParser::process_stream(const std::vector<uint8_t>& byte_stream) {
    std::vector<NaluPacket> data_list;
    size_t stream_size = byte_stream.size();

    // Prepend leftovers to the byte stream at the start of processing
    std::vector<uint8_t> full_stream = leftovers;
    full_stream.insert(full_stream.end(), byte_stream.begin(), byte_stream.end());

    size_t i = 0; // Start from the first possible occurrence of the stop_marker
    size_t last_processed_byte = i;
    uint8_t error_code = 0; // Initialize with no errors (0b00)

    while (i + packet_size <= full_stream.size()) {
        // Check if the stop_marker is in the correct place (the stop marker should be at the end of the packet)
        bool stop_marker_found = check_marker(full_stream, i + packet_size - stop_marker.size(), stop_marker);

        if (stop_marker_found) {
            // Calculate the position where the start_marker should be
            size_t start_marker_position = i;

            // Check if the start_marker is at the calculated position
            bool start_marker_found = check_marker(full_stream, start_marker_position, start_marker);

            if (start_marker_found) {
                // Create the packet from the byte stream (only the relevant part)
                std::vector<uint8_t> packet_data(full_stream.begin() + start_marker_position, full_stream.begin() + start_marker_position + packet_size);

                // Process packet
                process_packet(data_list, full_stream, start_marker_position, error_code);

                // Reset error code (no error)
                error_code = 0; // 0b00

                // Move to the next packet position
                i += packet_size;
                last_processed_byte = i;  // Update last processed byte
            } else {
                // Error handling: start_marker not found
                // Set the second least significant bit (bit 1) to 1
                error_code |= 0b10; // 0b10 indicates start marker error

                // Process packet with the error
                process_packet(data_list, full_stream, i, error_code);

                // Skip this packet and move to the next
                i += packet_size;
                last_processed_byte = i;  // Update last processed byte

                // Print detailed error message
                std::cerr << "Error! Start marker not found at expected position: " << start_marker_position << "\n";
                std::cerr << "Expected start marker: ";
                for (auto byte : start_marker) {
                    std::cerr << "0x" << std::hex << static_cast<int>(byte) << " ";
                }
                std::cerr << "\nActual data at position: ";
                for (size_t j = start_marker_position; j < start_marker_position + start_marker.size() && j < full_stream.size(); ++j) {
                    std::cerr << "0x" << std::hex << static_cast<int>(full_stream[j]) << " ";
                }
                std::cerr << "\n";
            }
        } else {
            // Error handling: stop_marker not found
            // Set the least significant bit (bit 0) to 1
            error_code |= 0b01; // 0b01 indicates stop marker error

            // Process packet with the error
            process_packet(data_list, full_stream, i, error_code);

            // Move to the next byte and keep looking for the stop marker
            i++;
        }
    }

    // Store leftovers if any
    leftovers.clear();  // Clear old leftovers before storing new ones
    if (last_processed_byte < full_stream.size()) {
        leftovers.assign(full_stream.begin() + last_processed_byte, full_stream.end());
    }

    return data_list;
}






// Process a byte stream and create a packet to append to the list
void NaluPacketParser::process_packet(std::vector<NaluPacket>& packets, 
                                      const std::vector<uint8_t>& byte_stream, 
                                      size_t start_index, 
                                      uint8_t error_code) {
    // Create a NaluPacket and populate its fields
    NaluPacket p;

    // Set the error code as info
    p.info = error_code;  // Directly assign the error code to info

    // Set the header to 0xAAAA
    p.header = 0xAAAA;

    // Increment parser_index and set it in the packet
    p.parser_index = packet_index++;
    packet_index %= UINT16_MAX; // Wrap around at UINT16_MAX

    size_t j = start_index + start_marker.size(); //STart at where the channel info should be

    // Extract the channel using the channel mask and shift
    p.channel = byte_stream[j] & chan_mask;  // Apply channel mask
    j += 1;

    // Extract the trigger times using the timing mask and shift
    uint16_t trigger_time_1 = (byte_stream[j] << 8) | byte_stream[j + 1];
    uint16_t trigger_time_2 = (byte_stream[j + 2] << 8) | byte_stream[j + 3];
    p.trigger_time = (trigger_time_1 << timing_shift) | (trigger_time_2 & timing_mask);
    j += 4;

    // Extract the logical and physical positions using the window and shift masks
    p.logical_position = ((byte_stream[j] & abs_wind_mask) << (8 - evt_wind_shift)) |
                         ((byte_stream[j + 1] >> evt_wind_shift) & evt_wind_mask);
    p.physical_position = byte_stream[j + 1] & abs_wind_mask;
    j += 2;

    //Copy data section over (no processing, can be processed later)
    std::memcpy(p.raw_samples, byte_stream.data() + j, 64);

    // Set footer to 0xFFFF
    p.footer = 0xFFFF;

    // Append the created packet to the list
    packets.push_back(p);
}

// Private method to check if the marker matches at the given index in the byte stream
bool NaluPacketParser::check_marker(const std::vector<uint8_t>& byte_stream, size_t index, const std::vector<uint8_t>& marker) {
    for (size_t j = 0; j < marker.size(); ++j) {
        if (byte_stream[index + j] != marker[j]) {
            return false; // Marker mismatch
        }
    }
    return true; // Marker found
}

