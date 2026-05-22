/**
 * @file logging.h
 * @brief Lightweight logging helpers backed by spdlog.
 */

#pragma once

#include <string_view>

#include <spdlog/common.h>

namespace nalu_event_collector::logging {

/** @brief Configure the default logger instance and global formatting. */
void configure(std::string_view level = "info");

/** @brief Change the global log level after configuration. */
void set_level(std::string_view level);

/** @brief Attach an additional sink to the collector logger/default logger. */
void add_sink(const spdlog::sink_ptr& sink);

}  // namespace nalu_event_collector::logging
