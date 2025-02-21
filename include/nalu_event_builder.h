#ifndef NALU_EVENT_BUILDER_H
#define NALU_EVENT_BUILDER_H

#include <vector>
#include <unordered_map>
#include <deque>
#include <cmath>    
#include "nalu_packet.h"
#include "nalu_event_wrapper.h"  

class NaluEventBuilder {
    public:
        NaluEventBuilder(int time_threshold = 5000, int windows = 10, 
                         std::vector<int> channels = {}, uint32_t max_trigger_time = (1 << 24)); // Default: 2^24
        ~NaluEventBuilder();
    
        void collect_events(const std::vector<NaluPacket>& packets);
        const std::vector<NaluEventWrapper>& get_events() const;
        void clean_events(size_t start_index);
        std::pair<std::vector<NaluEventWrapper>, size_t> get_events_after_timestamp(uint32_t timestamp);
        uint32_t compute_time_diff(uint32_t new_time, uint32_t old_time);

        // Setter for post_event_safety_buffer_size
        void set_post_event_safety_buffer_counter_max(size_t max_counter);
    
    private:
        int time_threshold_between_events;  
        int windows;                        
        std::vector<int> channels;          
    
        int event_index = 0;                
        uint32_t max_trigger_time;
        size_t event_max_packets;      
    
        std::vector<NaluEventWrapper> events;  
    
        // New variable to hold the number of packets to check after a new event is created
        size_t post_event_safety_buffer_counter_max;
        size_t post_event_safety_buffer_counter;
        bool in_safety_buffer_zone;
    
        void add_packet_to_event(const NaluPacket& packet);
        int find_event_index(uint32_t trigger_time);
        void manage_safety_buffer();
    };
    

#endif // NALU_EVENT_BUILDER_H
