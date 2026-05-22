/**
 * @file logging.cpp
 * @brief Implements the spdlog-backed logging helpers.
 */

#include "nalu_event_collector/logging/logging.h"

#include <stdexcept>
#include <string>
#include <vector>

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

namespace {

std::vector<spdlog::sink_ptr> extra_sinks;

bool has_sink(const std::shared_ptr<spdlog::logger>& logger, const spdlog::sink_ptr& sink) {
    if (!logger) {
        return false;
    }
    for (const auto& existing_sink : logger->sinks()) {
        if (existing_sink == sink) {
            return true;
        }
    }
    return false;
}

}  // namespace

namespace nalu_event_collector::logging {

void configure(std::string_view level) {
    if (spdlog::default_logger() == nullptr) {
        auto logger = spdlog::stdout_color_mt("nalu_event_collector");
        spdlog::set_default_logger(logger);
    }

    auto logger = spdlog::default_logger();
    for (const auto& sink : extra_sinks) {
        if (!has_sink(logger, sink)) {
            logger->sinks().push_back(sink);
        }
    }

    spdlog::set_pattern("%Y-%m-%d %H:%M:%S.%e [%^%l%$] %v");
    spdlog::set_level(parse_level(level));
    spdlog::flush_on(spdlog::level::warn);
}

void set_level(std::string_view level) { spdlog::set_level(parse_level(level)); }

void add_sink(const spdlog::sink_ptr& sink) {
    if (!sink) {
        return;
    }

    bool already_present = false;
    for (const auto& existing_sink : extra_sinks) {
        if (existing_sink == sink) {
            already_present = true;
            break;
        }
    }
    if (!already_present) {
        extra_sinks.push_back(sink);
    }

    auto logger = spdlog::default_logger();
    if (logger && !has_sink(logger, sink)) {
        logger->sinks().push_back(sink);
    }
}

}  // namespace nalu_event_collector::logging
