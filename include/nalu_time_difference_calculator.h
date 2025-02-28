#ifndef NALU_TIME_DIFFERENCE_CALCULATOR_H
#define NALU_TIME_DIFFERENCE_CALCULATOR_H

#include <cstdint>

class NaluTimeDifferenceCalculator {
public:
    NaluTimeDifferenceCalculator(uint32_t max_trigger_time, uint32_t time_threshold)
        : max_trigger_time(max_trigger_time), 
          half_max_trigger_time(max_trigger_time / 2),
          time_threshold(time_threshold) {}

    // Compute and return the adjusted time difference
    inline uint32_t compute_time_diff(uint32_t new_time, uint32_t old_time) const {
        uint32_t abs_diff = (new_time >= old_time) ? (new_time - old_time) : (old_time - new_time);
        return (abs_diff > half_max_trigger_time) ? (max_trigger_time - abs_diff) : abs_diff;
    }

    // Check if the computed time difference is within the threshold
    inline bool is_within_threshold(uint32_t new_time, uint32_t old_time) const {
        uint32_t time_diff = compute_time_diff(new_time, old_time);
        return time_diff <= time_threshold;
    }

private:
    uint32_t max_trigger_time;
    uint32_t half_max_trigger_time;
    uint32_t time_threshold;
};

#endif // NALU_TIME_DIFFERENCE_CALCULATOR_H
