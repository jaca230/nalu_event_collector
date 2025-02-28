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
                                   uint8_t timing_shift, bool check_packet_integrity,
                                   uint16_t constructed_packet_header, uint16_t constructed_packet_footer) 
    : packet_size(packet_size), chan_mask(chan_mask), chan_shift(chan_shift),
      abs_wind_mask(abs_wind_mask), evt_wind_mask(evt_wind_mask),
      evt_wind_shift(evt_wind_shift), timing_mask(timing_mask),
      timing_shift(timing_shift), start_marker(start_marker),
      stop_marker(stop_marker), check_packet_integrity(check_packet_integrity),
      constructed_packet_header(constructed_packet_header), constructed_packet_footer(constructed_packet_footer) {
        leftovers.reserve(packet_size); //pre-allocated space for leftovers
}

NaluPacketParser::NaluPacketParser(const NaluPacketParserParams& params)
    : NaluPacketParser(params.packet_size, params.start_marker, params.stop_marker,
                       params.chan_mask, params.chan_shift, params.abs_wind_mask, 
                       params.evt_wind_mask, params.evt_wind_shift, 
                       params.timing_mask, params.timing_shift, 
                       params.check_packet_integrity, params.constructed_packet_header, 
                       params.constructed_packet_footer) {
    // Additional setup logic if needed
}


std::vector<NaluPacket> NaluPacketParser::process_stream(const std::vector<uint8_t>& byte_stream) {
    std::vector<NaluPacket> data_list;
    size_t stream_size = byte_stream.size();

    const uint8_t* data_ptr = byte_stream.data(); // Use a raw pointer to access the byte stream
    size_t last_processed_byte = 0;
    uint8_t error_code = 0; // Initialize with no errors (0b00)

    size_t stop_marker_len = stop_marker.size();
    size_t start_marker_len = start_marker.size();
    size_t leftovers_size = leftovers.size();
    size_t byte_stream_size = byte_stream.size();

    size_t i = 0;

    // Handle creating first packet from leftovers and byte stream
    if (leftovers_size > 0) {
        process_leftovers(data_list, data_ptr, byte_stream_size, leftovers_size, packet_size, start_marker_len, stop_marker_len);
        i = packet_size - (leftovers_size); //Move index to be start of first packet fully contained in byte_stream
    }

    // Always process first packets with checks. This handles if we're initially misaligned (i.e. started while reciever running)
    // Under normal operation this shouldn't be necessary, but it's a small overhead anyways
    size_t data_list_size = data_list.size();
    while (i + packet_size <= byte_stream.size()) {
        process_byte_stream_segment_with_checks(data_list, data_ptr, i, error_code, start_marker_len, stop_marker_len);
    
        // If the size of data_list has increased, break out of the loop
        if (data_list.size() > data_list_size) {
            break;
        }
    }
    

    // Define function pointer based on the check_packet_integrity flag
    void (NaluPacketParser::*process_segment)(std::vector<NaluPacket>&, const uint8_t*, size_t&, uint8_t&, size_t, size_t);

    // Assign the function pointer
    if (check_packet_integrity) {
        process_segment = &NaluPacketParser::process_byte_stream_segment_with_checks;
    } else {
        process_segment = &NaluPacketParser::process_byte_stream_segment_without_checks;
    }

    // Process the byte stream in segments
    while (i + packet_size <= byte_stream.size()) {
        (this->*process_segment)(data_list, data_ptr, i, error_code, start_marker_len, stop_marker_len);
    }

    // Store leftovers if any
    leftovers.clear();
    if (i < byte_stream.size()) {
        leftovers.assign(byte_stream.begin() + i, byte_stream.end());
    }


    return data_list;
}



// Optimized packet processing without copying unnecessary data
void NaluPacketParser::process_packet(std::vector<NaluPacket>& packets, 
                                      const uint8_t* byte_stream, 
                                      size_t start_index, 
                                      uint8_t error_code) {
    NaluPacket p;
    p.info = error_code;
    p.header = constructed_packet_header;
    p.parser_index = packet_index++;
    packet_index %= UINT16_MAX;

    size_t j = start_index + start_marker.size(); // Start at the channel info

    p.channel = byte_stream[j] & chan_mask;
    j++;

    uint16_t trigger_time_1 = (byte_stream[j] << 8) | byte_stream[j + 1];
    uint16_t trigger_time_2 = (byte_stream[j + 2] << 8) | byte_stream[j + 3];
    p.trigger_time = (trigger_time_1 << timing_shift) | (trigger_time_2 & timing_mask);
    j += 4;

    p.logical_position = ((byte_stream[j] & abs_wind_mask) << (8 - evt_wind_shift)) |
                         ((byte_stream[j + 1] >> evt_wind_shift) & evt_wind_mask);
    p.physical_position = byte_stream[j + 1] & abs_wind_mask;
    j += 2;

    std::memcpy(p.raw_samples, byte_stream + j, 64); // No data copy here, directly use pointers

    p.footer = constructed_packet_footer;
    packets.push_back(p);
}


bool NaluPacketParser::check_marker(const uint8_t* byte_stream, size_t index, const uint8_t* marker, size_t marker_len) {
    for (size_t j = 0; j < marker_len; ++j) {
        if (byte_stream[index + j] != marker[j]) {
            return false; // Marker mismatch
        }
    }
    return true; // Marker found
}

// Private helper method: processes the byte stream segment with marker checks
void NaluPacketParser::process_byte_stream_segment_with_checks(std::vector<NaluPacket>& packets,
                                                               const uint8_t* byte_stream,
                                                               size_t& i, 
                                                               uint8_t& error_code,
                                                               size_t start_marker_len,
                                                               size_t stop_marker_len) {
    size_t end_marker_position = i + packet_size - stop_marker_len;
    if (check_marker(byte_stream, end_marker_position, stop_marker.data(), stop_marker_len)) {
        size_t start_marker_position = i;

        // Check for start_marker at the calculated position
        if (check_marker(byte_stream, start_marker_position, start_marker.data(), start_marker_len)) {
            // Create the packet from the byte stream (only the relevant part)
            process_packet(packets, byte_stream, start_marker_position, error_code);

            // Reset error code (no error)
            error_code = 0; // 0b00
            i += packet_size; // Move to the next packet position
        } else {
            error_code |= 0b10; // 0b10 indicates start marker error
            process_packet(packets, byte_stream, i, error_code);
            i += packet_size;
            std::cerr << "WARNING: Start marker not found at expected position.\n";
        }
    } else {
        error_code |= 0b01; // 0b01 indicates stop marker error
        i++; // Move to the next byte
    }
}


// Private helper method: processes the byte stream segment without marker checks
void NaluPacketParser::process_byte_stream_segment_without_checks(std::vector<NaluPacket>& packets,
                                                                  const uint8_t* byte_stream,
                                                                  size_t& i, 
                                                                  uint8_t& error_code,     //Unused
                                                                  size_t start_marker_len, // Unused
                                                                  size_t stop_marker_len) { // Unused
    size_t start_marker_position = i;
    
    // Create the packet from the byte stream (only the relevant part)
    process_packet(packets, byte_stream, start_marker_position, 0);

    // No marker checks needed, simply move to the next packet position
    i += packet_size; // Move to the next packet position
}

// Private helper method: processes the leftovers and byte stream
// Private helper method: processes the leftovers and byte stream
void NaluPacketParser::process_leftovers(std::vector<NaluPacket>& data_list, 
                                          const uint8_t* byte_stream, 
                                          size_t byte_stream_len,
                                          size_t leftovers_size, 
                                          const size_t packet_size,
                                          const size_t start_marker_len,
                                          const size_t stop_marker_len) {
    if (leftovers_size >= packet_size) {
        std::cout << "WARNING: Leftovers size (" << leftovers_size 
                  << ") is greater than or equal to the packet size (" << packet_size 
                  << "). \n";
    } else {
        // Calculate how many bytes from byte_stream are needed to complete the packet
        size_t remaining_bytes = packet_size - leftovers_size;

        // Resize leftovers to ensure it has enough space for a full packet
        leftovers.resize(packet_size);

        // Step 1: Copy the remaining bytes from byte_stream into the leftover space
        std::memcpy(leftovers.data() + leftovers_size, byte_stream, remaining_bytes);

        // Now leftovers contains the full packet data (or as much as is available)
        const uint8_t* combined_ptr = leftovers.data();

        // Check for start_marker at the beginning of the combined data
        if (!check_marker(combined_ptr, 0, start_marker.data(), start_marker_len)) {
            std::cout << "WARNING: Combined data does not start with the start marker.\n";
        }

        // Check if the byte stream has the stop marker in the correct position
        size_t end_marker_position = packet_size - stop_marker_len;
        if (end_marker_position <= byte_stream_len) {
            if (check_marker(combined_ptr, end_marker_position, stop_marker.data(), stop_marker_len)) {
                process_packet(data_list, combined_ptr, 0, 0);  // Process the full packet
            } else {
                std::cout << "WARNING: Byte stream does not have stop marker at expected position.\n";
            }
        } else {
            std::cout << "WARNING: Byte stream is too short to check stop marker at expected position.\n";
        }
    }
}








