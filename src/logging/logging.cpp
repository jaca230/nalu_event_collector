/**
 * @file logging.cpp
 * @brief Implements the spdlog-backed logging helpers.
 */

#include "nalu_event_collector/logging/logging.h"

#include <stdexcept>
#include <string>

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace {

spdlog::level::level_enum parse_level(std::string_view level) {
    if (level == "debug") {
        return spdlog::level::debug;
    }
    if (level == "info") {
        return spdlog::level::info;
    }
    if (level == "warning" || level == "warn") {
        return spdlog::level::warn;
    }
    if (level == "error") {
        return spdlog::level::err;
    }
    if (level == "critical") {
        return spdlog::level::critical;
    }
    throw std::invalid_argument("Invalid log level: " + std::string(level));
}

}  // namespace

namespace nalu_event_collector::logging {

void configure(std::string_view level) {
    if (spdlog::default_logger() == nullptr) {
        auto logger = spdlog::stdout_color_mt("nalu_event_collector");
        spdlog::set_default_logger(logger);
    }

    spdlog::set_pattern("%Y-%m-%d %H:%M:%S.%e [%^%l%$] %v");
    spdlog::set_level(parse_level(level));
    spdlog::flush_on(spdlog::level::warn);
}

void set_level(std::string_view level) { spdlog::set_level(parse_level(level)); }

}  // namespace nalu_event_collector::logging
