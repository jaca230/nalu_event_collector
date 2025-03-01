#ifndef NALU_EVENT_COLLECTOR_LOGGER_H
#define NALU_EVENT_COLLECTOR_LOGGER_H

#include <iostream>
#include <fstream>
#include <string>

class NaluEventCollectorLogger {
public:
    enum class LogLevel { DEBUG, INFO, WARNING, ERROR };

    static void set_level(LogLevel level);
    static void enable_file_logging(const std::string& filename);
    static void disable_file_logging();
    static void set_log_colors(const std::string& debug_color, const std::string& info_color,
                               const std::string& warning_color, const std::string& error_color);

    static void debug(const std::string& message);
    static void info(const std::string& message);
    static void warning(const std::string& message);
    static void error(const std::string& message);

private:
    static void log(const std::string& prefix, const std::string& message, LogLevel level);
    static std::string get_colored_message(const std::string& message, const std::string& color);
    static std::string get_color_for_level(LogLevel level);

    static LogLevel log_level;
    static std::ofstream log_file;

    static std::string debug_color;
    static std::string info_color;
    static std::string warning_color;
    static std::string error_color;
};

#endif // NALU_EVENT_COLLECTOR_LOGGER_H
