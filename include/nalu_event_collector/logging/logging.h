/**
 * @file logging.h
 * @brief Lightweight logging helpers backed by spdlog.
 */

#pragma once

#include <string_view>

namespace nalu_event_collector::logging {

/** @brief Configure the default logger instance and global formatting. */
void configure(std::string_view level = "info");

/** @brief Change the global log level after configuration. */
void set_level(std::string_view level);

}  // namespace nalu_event_collector::logging
