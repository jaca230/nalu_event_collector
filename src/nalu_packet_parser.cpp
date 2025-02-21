#include "nalu_packet_parser.h"
#include <algorithm>  // For std::search
#include <stdexcept>  // For exceptions
#include <cstring>    // For std::memcpy

// Constructor
NaluPacketParser::NaluPacketParser(size_t packet_size, 
                                   uint8_t start_marker, 
                                   const std::vector<uint8_t>& stop_marker, 
                                   size_t scrub_length, 
                                   size_t error_correction_step_size, 
                                   size_t error_correction_max_steps, 
                                   const std::vector<uint8_t>& error_bytes)
    : packet_size(packet_size),
      start_marker(start_marker),
      stop_marker(stop_marker),
      scrub_length(scrub_length),
      error_correction_step_size(error_correction_step_size),
      error_correction_max_steps(error_correction_max_steps),
      error_bytes(error_bytes) {}

// Getters and setters
size_t NaluPacketParser::get_packet_size() const {
    return packet_size;
}

void NaluPacketParser::set_packet_size(size_t packet_size) {
    this->packet_size = packet_size;
}

uint8_t NaluPacketParser::get_start_marker() const {
    return start_marker;
}

void NaluPacketParser::set_start_marker(uint8_t start_marker) {
    this->start_marker = start_marker;
}

std::vector<uint8_t> NaluPacketParser::get_stop_marker() const {
    return stop_marker;
}

void NaluPacketParser::set_stop_marker(const std::vector<uint8_t>& stop_marker) {
    this->stop_marker = stop_marker;
}

size_t NaluPacketParser::get_scrub_length() const {
    return scrub_length;
}

void NaluPacketParser::set_scrub_length(size_t scrub_length) {
    this->scrub_length = scrub_length;
}

size_t NaluPacketParser::get_error_correction_step_size() const {
    return error_correction_step_size;
}

void NaluPacketParser::set_error_correction_step_size(size_t error_correction_step_size) {
    this->error_correction_step_size = error_correction_step_size;
}

size_t NaluPacketParser::get_error_correction_max_steps() const {
    return error_correction_max_steps;
}

void NaluPacketParser::set_error_correction_max_steps(size_t error_correction_max_steps) {
    this->error_correction_max_steps = error_correction_max_steps;
}

std::vector<uint8_t> NaluPacketParser::get_error_bytes() const {
    return error_bytes;
}

void NaluPacketParser::set_error_bytes(const std::vector<uint8_t>& error_bytes) {
    this->error_bytes = error_bytes;
}

// Scrub the byte stream by removing error bytes
std::vector<uint8_t> NaluPacketParser::scrub_byte_stream(const std::vector<uint8_t>& byte_stream) {
    if (error_bytes.empty()) return byte_stream; // No error pattern means no scrubbing needed

    std::vector<uint8_t> scrubbed_stream;
    scrubbed_stream.reserve(byte_stream.size()); // Preallocate memory

    auto it = byte_stream.begin();
    while (it != byte_stream.end()) {
        auto found = std::search(it, byte_stream.end(), error_bytes.begin(), error_bytes.end());

        if (found == byte_stream.end()) {
            scrubbed_stream.insert(scrubbed_stream.end(), it, byte_stream.end());
            break;
        }

        scrubbed_stream.insert(scrubbed_stream.end(), it, found);
        it = found + error_bytes.size();
    }

    return scrubbed_stream;
}

// Handle scrubbed segment and add to packets if valid
void NaluPacketParser::handle_scrubbed_segment(std::vector<uint8_t>& scrubbed_segment, 
                                               std::vector<std::vector<uint8_t>>& packets, 
                                               size_t error_packet_start,
                                               uint8_t error_code) {
    size_t start_index = scrubbed_segment.size() - packet_size;

    // Ensure start word is at the computed start index
    if (scrubbed_segment[start_index] == start_marker) {
        append_error_code(packets, scrubbed_segment, start_index, error_code);
        return;
    }

    // Handle malformed packet
    std::cerr << "Malformed packet at: " << error_packet_start
              << "\nMalformed packet length: " << scrubbed_segment.size() << " bytes"
              << "\nMalformed packet bytes: ";

    for (uint8_t byte : scrubbed_segment) {
        std::cerr << std::hex << static_cast<int>(byte) << " ";
    }
    std::cerr << std::dec << std::endl;
}

// Append the error code to the packet data
void NaluPacketParser::append_error_code(std::vector<std::vector<uint8_t>>& packets, 
                       const std::vector<uint8_t>& byte_stream, 
                       size_t start_index, 
                       uint8_t error_code) {
    // Create a vector with space for the error code + packet data
    std::vector<uint8_t> packet_with_error(packet_size + 1);

    // Set the error code at the beginning
    packet_with_error[0] = error_code;

    // Copy the packet data from the processed stream into the remaining space
    std::memcpy(packet_with_error.data() + 1, byte_stream.data() + start_index, packet_size);

    // Add the modified packet to the packets list
    packets.push_back(std::move(packet_with_error));
}


// Split packets from the byte stream
std::vector<std::vector<uint8_t>> NaluPacketParser::split_packets(const std::vector<uint8_t>& initial_byte_stream) {

    // We should only need to do this for the very beginning of the byte stream
    // This is a consequence of "error byte sequences", we need to scrub the stream of error bytes
    // and then look for the first occurence of the stop word with the start word behind it in the
    // expected location.
    std::vector<uint8_t> scrubbed_prefix = scrub_byte_stream(std::vector<uint8_t>(initial_byte_stream.begin(), 
                                                             initial_byte_stream.begin() + scrub_length));

    std::vector<uint8_t> processed_stream = scrubbed_prefix;
    processed_stream.insert(processed_stream.end(), 
                            initial_byte_stream.begin() + scrub_length, 
                            initial_byte_stream.end());

    std::vector<std::vector<uint8_t>> packets;
    size_t stream_size = processed_stream.size();
    size_t i = packet_size - 2; // Start searching from the first full packet offset

    while (i + 1 < stream_size) {
        if (processed_stream[i] == stop_marker[0] && processed_stream[i + 1] == stop_marker[1]) {
            size_t start_index = i - packet_size + 2;

            if (processed_stream[start_index] == start_marker) {
                uint8_t error_code = 0;
                append_error_code(packets, processed_stream, start_index, error_code);
                break;
            }
        }
        i++;
    }

    // --- Extract Remaining Packets ---
    while (i + packet_size + 1 < stream_size) {
        if (processed_stream[i + packet_size] == stop_marker[0] &&
            processed_stream[i + packet_size + 1] == stop_marker[1]) {
            // Nothing went wrong
            uint8_t error_code = 0;
            append_error_code(packets, processed_stream, i + 2, error_code);
            i += packet_size;
        } else {
            // Begin Error Correction
            size_t error_packet_start = i;
            size_t search_index = error_packet_start;
            int error_steps_taken = 0;
            bool found_valid_stop = false;

            // Fast Error Correction
            while (error_steps_taken < error_correction_max_steps && search_index + packet_size + 1 < stream_size) {
                // "Error byte sequences" come in set sizes (generally 16 bytes), so we expect the stop word to be
                // 16 * m bytes away where m is an integer (number of "error byte sequences")
                // We only try this a set number of times (default 5), as it would be very unlikely
                // for many "error byte sequences" to be in one packet
                search_index += error_correction_step_size;
                error_steps_taken++;

                if (processed_stream[search_index + packet_size] == stop_marker[0] &&
                    processed_stream[search_index + packet_size + 1] == stop_marker[1]) {
                    std::vector<uint8_t> error_segment(processed_stream.begin() + error_packet_start + 2,
                                                       processed_stream.begin() + search_index + packet_size + 2);

                    std::vector<uint8_t> scrubbed_segment = scrub_byte_stream(error_segment);

                    uint8_t error_code = 1;
                    handle_scrubbed_segment(scrubbed_segment, packets, error_packet_start, error_code);

                    i = search_index + packet_size;
                    found_valid_stop = true;
                    break;
                }
            }

            // Brute Force Error Correction
            if (!found_valid_stop) {
                while (i + packet_size + 1 < stream_size) {
                    //We progress 1 byte at a time until we find the next stop marker
                    i++;

                    if (processed_stream[i + packet_size] == stop_marker[0] &&
                        processed_stream[i + packet_size + 1] == stop_marker[1]) {
                        std::vector<uint8_t> error_segment(processed_stream.begin() + error_packet_start + 2,
                                                           processed_stream.begin() + i + packet_size + 2);

                        std::vector<uint8_t> scrubbed_segment = scrub_byte_stream(error_segment);
                        
                        uint8_t error_code = 2;
                        handle_scrubbed_segment(scrubbed_segment, packets, error_packet_start, error_code);

                        i += packet_size;
                        break;
                    }
                }
            }
        }
    }

    return packets;
}

// Parse the packets and convert them into NaluPacket objects
std::vector<NaluPacket> NaluPacketParser::parse_packet(const std::vector<std::vector<uint8_t>>& packets) {
    std::vector<NaluPacket> data_list;
    data_list.reserve(packets.size());
    int packet_size_with_extra_info = packet_size + 1;

    for (size_t i = 0; i < packets.size(); ++i) {
        const auto& packet = packets[i];

        if (packet.size() != packet_size_with_extra_info) {
            std::cerr << "Skipped packet with invalid length: " << packet.size() << std::endl;
            continue;
        }

        NaluPacket p;
        p.header = 0xAAAA;  // Set header to 0xAAAA

        p.parser_index = packet_index++;  // Set parser_index and increment it
        packet_index %= UINT16_MAX; // Loop around when it overflows

        size_t j = 0; 
        p.info = packet[0];
        j += 2; //Skip the header byte

        p.channel = packet[j++] & 0x3F;

        uint16_t trigger_time_1 = (packet[j] << 8) | packet[j + 1];
        uint16_t trigger_time_2 = (packet[j + 2] << 8) | packet[j + 3];
        p.trigger_time = (trigger_time_1 << 12) | (trigger_time_2 & 0xFFF);
        j += 4;

        p.logical_position = ((packet[j] & 0x3F) << 2) | ((packet[j + 1] >> 6) & 0x03);
        p.physical_position = packet[j + 1] & 0x3F;
        j += 2;

        std::memcpy(p.raw_samples, &packet[j], 64);
        j += 64;

        data_list.push_back(p);
        
        p.footer = 0xFFFF;  // Set footer to 0xFFFF
    }

    return data_list;
}


