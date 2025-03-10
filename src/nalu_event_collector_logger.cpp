#include "nalu_event_collector_logger.h"
#include <map>

NaluEventCollectorLogger::LogLevel NaluEventCollectorLogger::log_level =
    NaluEventCollectorLogger::LogLevel::INFO;
std::ofstream NaluEventCollectorLogger::log_file;

std::string NaluEventCollectorLogger::debug_color = "\033[0;37m";    // White
std::string NaluEventCollectorLogger::info_color = "\033[0;37m";     // White
std::string NaluEventCollectorLogger::warning_color = "\033[1;33m";  // Yellow
std::string NaluEventCollectorLogger::error_color = "\033[1;31m";    // Red

void NaluEventCollectorLogger::set_level(LogLevel level) { log_level = level; }

void NaluEventCollectorLogger::set_level(const std::string& level) {
    static std::map<std::string, LogLevel> level_map = {
        {"debug", LogLevel::DEBUG},
        {"info", LogLevel::INFO},
        {"warning", LogLevel::WARNING},
        {"error", LogLevel::ERROR},
    };

    auto it = level_map.find(level);
    if (it != level_map.end()) {
        log_level = it->second;
    } else {
        throw std::invalid_argument("Invalid log level string: " + level);
    }
}

void NaluEventCollectorLogger::enable_file_logging(
    const std::string& filename) {
    log_file.open(filename, std::ios::out | std::ios::app);
    if (!log_file) {
        std::cerr << "[ERROR] Failed to open log file: " << filename
                  << std::endl;
    }
}

void NaluEventCollectorLogger::disable_file_logging() {
    if (log_file.is_open()) log_file.close();
}

void NaluEventCollectorLogger::set_log_colors(const std::string& debug_color,
                                              const std::string& info_color,
                                              const std::string& warning_color,
                                              const std::string& error_color) {
    NaluEventCollectorLogger::debug_color = debug_color;
    NaluEventCollectorLogger::info_color = info_color;
    NaluEventCollectorLogger::warning_color = warning_color;
    NaluEventCollectorLogger::error_color = error_color;
}

void NaluEventCollectorLogger::debug(const std::string& message) {
    log("[DEBUG] ", message, LogLevel::DEBUG);
}

void NaluEventCollectorLogger::info(const std::string& message) {
    log("[INFO]  ", message, LogLevel::INFO);
}

void NaluEventCollectorLogger::warning(const std::string& message) {
    log("[WARN]  ", message, LogLevel::WARNING);
}

void NaluEventCollectorLogger::error(const std::string& message) {
    log("[ERROR] ", message, LogLevel::ERROR);
}

void NaluEventCollectorLogger::log(const std::string& prefix,
                                   const std::string& message, LogLevel level) {
    if (level >= log_level) {
        std::string log_message = prefix + message;
        std::string colored_message =
            get_colored_message(log_message, get_color_for_level(level));
        std::cout << colored_message << std::endl;
        if (log_file.is_open()) {
            log_file << log_message << std::endl;
        }
    }
}

std::string NaluEventCollectorLogger::get_colored_message(
    const std::string& message, const std::string& color) {
    return color + message + "\033[0m";
}

std::string NaluEventCollectorLogger::get_color_for_level(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:
            return debug_color;
        case LogLevel::INFO:
            return info_color;
        case LogLevel::WARNING:
            return warning_color;
        case LogLevel::ERROR:
            return error_color;
        default:
            return "\033[0m";
    }
}
