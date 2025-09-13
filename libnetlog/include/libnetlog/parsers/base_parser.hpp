#pragma once

#include "../log_entry.hpp"
#include "../device_types.hpp"
#include <memory>
#include <vector>
#include <string>
#include <optional>

namespace libnetlog {

/**
 * @brief Abstract base class for all log parsers
 * 
 * This defines the interface that all device-specific parsers must implement.
 * Each parser is responsible for understanding the specific log format of
 * a particular device type and converting raw log messages into LogEntry objects.
 */
class BaseParser {
public:
    virtual ~BaseParser() = default;

    /**
     * @brief Parse a single raw log message
     * 
     * @param raw_message The raw log message to parse
     * @return LogEntry if parsing successful, std::nullopt if parsing failed
     */
    virtual std::optional<LogEntry> parse(const std::string& raw_message) = 0;

    /**
     * @brief Parse multiple raw log messages
     * 
     * @param raw_messages Vector of raw log messages
     * @return Vector of successfully parsed LogEntry objects
     */
    virtual std::vector<LogEntry> parse_batch(const std::vector<std::string>& raw_messages);

    /**
     * @brief Check if this parser can handle the given raw message
     * 
     * @param raw_message The raw log message to check
     * @return true if this parser can likely handle the message, false otherwise
     */
    virtual bool can_parse(const std::string& raw_message) const = 0;

    /**
     * @brief Get the device type this parser handles
     * 
     * @return DeviceType that this parser is designed for
     */
    virtual DeviceType get_device_type() const = 0;

    /**
     * @brief Get the parser name/identifier
     * 
     * @return String identifier for this parser
     */
    virtual std::string get_parser_name() const = 0;

    /**
     * @brief Get parser version
     * 
     * @return Version string for this parser
     */
    virtual std::string get_version() const { return "1.0.0"; }

    /**
     * @brief Get supported log format patterns
     * 
     * @return Vector of regex patterns this parser recognizes
     */
    virtual std::vector<std::string> get_supported_patterns() const = 0;

protected:
    /**
     * @brief Helper method to create a basic LogEntry with common fields
     * 
     * @param timestamp Parsed timestamp
     * @param severity Parsed severity
     * @param message Parsed message content
     * @param raw_message Original raw message
     * @return LogEntry with basic fields populated
     */
    LogEntry create_log_entry(LogEntry::timestamp_t timestamp,
                             Severity severity,
                             const std::string& message,
                             const std::string& raw_message) const;

    /**
     * @brief Helper method to parse timestamp from various formats
     * 
     * @param timestamp_str String representation of timestamp
     * @return Parsed timestamp or current time if parsing fails
     */
    LogEntry::timestamp_t parse_timestamp(const std::string& timestamp_str) const;

    /**
     * @brief Helper method to extract hostname from log message
     * 
     * @param message Log message that may contain hostname
     * @return Extracted hostname or empty string
     */
    std::string extract_hostname(const std::string& message) const;

    /**
     * @brief Helper method to clean up log message content
     * 
     * @param message Raw message content
     * @return Cleaned message content
     */
    std::string clean_message(const std::string& message) const;
};

} // namespace libnetlog