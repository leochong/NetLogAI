#pragma once

#include "base_parser.hpp"
#include <regex>

namespace libnetlog {

/**
 * @brief Parser for generic RFC 3164/5424 syslog messages
 * 
 * Handles standard syslog format messages that don't match
 * any device-specific parser. This is the fallback parser.
 */
class GenericSyslogParser : public BaseParser {
public:
    GenericSyslogParser();
    ~GenericSyslogParser() override = default;

    std::optional<LogEntry> parse(const std::string& raw_message) override;
    bool can_parse(const std::string& raw_message) const override;
    DeviceType get_device_type() const override { return DeviceType::GenericSyslog; }
    std::string get_parser_name() const override { return "Generic Syslog Parser"; }
    std::vector<std::string> get_supported_patterns() const override;

private:
    std::regex rfc3164_pattern_;  // Traditional syslog format
    std::regex rfc5424_pattern_;  // New syslog format
    std::regex priority_pattern_; // Priority extraction
};

} // namespace libnetlog