#ifndef NALU_TIME_DIFFERENCE_CALCULATOR_H
#define NALU_TIME_DIFFERENCE_CALCULATOR_H

#include <cstdint>

/// @class NaluTimeDifferenceCalculator
/// @brief This class computes time differences based on the clock cycles of the board.
/// It also checks if the time difference is within a specified threshold, taking into account a maximum trigger time
/// and its half value to handle wraparounds in the clock cycle counts.
class NaluTimeDifferenceCalculator {
public:
    /// @brief Constructor to initialize the time difference calculator.
    /// @param max_trigger_time The maximum trigger time (in clock cycles).
    /// @param time_threshold The maximum allowable time difference (in clock cycles) for a valid event.
    NaluTimeDifferenceCalculator(uint32_t max_trigger_time, uint32_t time_threshold)
        : max_trigger_time(max_trigger_time), 
          half_max_trigger_time(max_trigger_time / 2),
          time_threshold(time_threshold) {}

    /// @brief Compute the adjusted time difference between two time values in clock cycles.
    /// The calculation considers potential wraparound in the clock cycles.
    /// @param new_time The newer time value (in clock cycles).
    /// @param old_time The older time value (in clock cycles).
    /// @return The adjusted time difference (in clock cycles).
    inline uint32_t compute_time_diff(uint32_t new_time, uint32_t old_time) const {
        uint32_t abs_diff = (new_time >= old_time) ? (new_time - old_time) : (old_time - new_time);
        return (abs_diff > half_max_trigger_time) ? (max_trigger_time - abs_diff) : abs_diff;
    }

    /// @brief Check if the computed time difference between two time values is within the specified threshold.
    /// @param new_time The newer time value (in clock cycles).
    /// @param old_time The older time value (in clock cycles).
    /// @return True if the time difference is within the threshold, false otherwise.
    inline bool is_within_threshold(uint32_t new_time, uint32_t old_time) const {
        uint32_t time_diff = compute_time_diff(new_time, old_time);
        return time_diff <= time_threshold;
    }

private:
    uint32_t max_trigger_time; ///< The maximum trigger time (in clock cycles).
    uint32_t half_max_trigger_time; ///< Half of the maximum trigger time (in clock cycles).
    uint32_t time_threshold; ///< The maximum allowed time difference (in clock cycles) for two packets to be grouped into an event.
};

#endif // NALU_TIME_DIFFERENCE_CALCULATOR_H
