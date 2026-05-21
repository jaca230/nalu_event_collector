/**
 * @file time_difference_calculator.h
 * @brief Utility for trigger-time comparisons with counter wraparound support.
 */

#pragma once

#include <cstdint>

namespace nalu_event_collector {

/**
 * @brief Computes trigger-time differences while accounting for wraparound.
 */
class TimeDifferenceCalculator {
  public:
    /** @brief Construct a calculator from the counter range and match threshold. */
    TimeDifferenceCalculator(uint32_t max_trigger_time, uint32_t time_threshold)
        : max_trigger_time_(max_trigger_time),
          half_max_trigger_time_(max_trigger_time / 2),
          time_threshold_(time_threshold) {}

    /** @brief Return the wrapped absolute difference between two trigger times. */
    uint32_t compute_time_diff(uint32_t new_time, uint32_t old_time) const {
        const uint32_t abs_diff =
            (new_time >= old_time) ? (new_time - old_time) : (old_time - new_time);
        return (abs_diff > half_max_trigger_time_)
                   ? (max_trigger_time_ - abs_diff)
                   : abs_diff;
    }

    /** @brief Return true when two trigger times are within the configured threshold. */
    bool is_within_threshold(uint32_t new_time, uint32_t old_time) const {
        return compute_time_diff(new_time, old_time) <= time_threshold_;
    }

  private:
    uint32_t max_trigger_time_;
    uint32_t half_max_trigger_time_;
    uint32_t time_threshold_;
};

}  // namespace nalu_event_collector
