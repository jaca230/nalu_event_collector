#ifndef NALU_EVENT_COLLECTOR_LOGGER_H
#define NALU_EVENT_COLLECTOR_LOGGER_H

#include <iostream>
#include <fstream>
#include <string>

/**
 * @brief A logger class for the NaluEventCollector.
 * 
 * This class provides logging functionality with different log levels (DEBUG, INFO, WARNING, ERROR), 
 * allowing logs to be output to both the console (with optional color formatting) and a log file. 
 * The logging behavior can be customized by adjusting the log level, enabling/disabling file logging, 
 * and changing the color scheme for each log level.
 */
class NaluEventCollectorLogger {
public:
    /**
     * @brief Enumeration for log levels.
     * 
     * The log levels determine the verbosity of the logs:
     * - DEBUG: Detailed information, usually for debugging.
     * - INFO: General information messages.
     * - WARNING: Warnings about potential issues.
     * - ERROR: Errors that indicate failures or critical issues.
     */
    enum class LogLevel { DEBUG, INFO, WARNING, ERROR };

    /**
     * @brief Sets the minimum log level for the logger.
     * 
     * Only log messages with a level greater than or equal to the specified level will be logged.
     * 
     * @param level The minimum log level (DEBUG, INFO, WARNING, ERROR).
     */
    static void set_level(LogLevel level);

    /**
     * @brief Enables logging to a file.
     * 
     * Log messages will be appended to the specified file.
     * 
     * @param filename The name of the log file.
     */
    static void enable_file_logging(const std::string& filename);

    /**
     * @brief Disables logging to the file.
     * 
     * If file logging is enabled, this function will close the log file.
     */
    static void disable_file_logging();

    /**
     * @brief Sets custom colors for log messages based on log levels.
     * 
     * @param debug_color The color for debug messages.
     * @param info_color The color for info messages.
     * @param warning_color The color for warning messages.
     * @param error_color The color for error messages.
     */
    static void set_log_colors(const std::string& debug_color, const std::string& info_color,
                               const std::string& warning_color, const std::string& error_color);

    /**
     * @brief Logs a debug-level message.
     * 
     * @param message The message to be logged.
     */
    static void debug(const std::string& message);

    /**
     * @brief Logs an info-level message.
     * 
     * @param message The message to be logged.
     */
    static void info(const std::string& message);

    /**
     * @brief Logs a warning-level message.
     * 
     * @param message The message to be logged.
     */
    static void warning(const std::string& message);

    /**
     * @brief Logs an error-level message.
     * 
     * @param message The message to be logged.
     */
    static void error(const std::string& message);

private:
    /**
     * @brief A private function to log messages based on the level and prefix.
     * 
     * This function is used internally by all log-level specific functions.
     * 
     * @param prefix The log level prefix (e.g., "[DEBUG]", "[INFO]").
     * @param message The message to be logged.
     * @param level The log level for the message.
     */
    static void log(const std::string& prefix, const std::string& message, LogLevel level);

    /**
     * @brief Adds color formatting to the log message based on the log level.
     * 
     * @param message The log message to format.
     * @param color The color to apply to the log message.
     * @return A string containing the colorized log message.
     */
    static std::string get_colored_message(const std::string& message, const std::string& color);

    /**
     * @brief Retrieves the color for a specific log level.
     * 
     * @param level The log level.
     * @return The color associated with the log level.
     */
    static std::string get_color_for_level(LogLevel level);

    static LogLevel log_level;         ///< The current log level for the logger.
    static std::ofstream log_file;     ///< The file stream for logging to a file.

    static std::string debug_color;    ///< The color for debug messages.
    static std::string info_color;     ///< The color for info messages.
    static std::string warning_color;  ///< The color for warning messages.
    static std::string error_color;    ///< The color for error messages.
};

#endif // NALU_EVENT_COLLECTOR_LOGGER_H
