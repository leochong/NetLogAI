#pragma once

#include "base_parser.hpp"
#include <regex>

namespace libnetlog {

/**
 * @brief Parser for Cisco IOS and IOS-XE log messages
 * 
 * Handles various Cisco IOS log formats including:
 * - Standard syslog format with facility/severity
 * - Timestamped messages
 * - Interface up/down notifications
 * - BGP, OSPF, and other protocol messages
 * - System event messages
 */
class CiscoIOSParser : public BaseParser {
public:
    CiscoIOSParser();
    ~CiscoIOSParser() override = default;

    std::optional<LogEntry> parse(const std::string& raw_message) override;
    bool can_parse(const std::string& raw_message) const override;
    DeviceType get_device_type() const override { return DeviceType::CiscoIOS; }
    std::string get_parser_name() const override { return "Cisco IOS Parser"; }
    std::vector<std::string> get_supported_patterns() const override;

private:
    /**
     * @brief Parse standard Cisco IOS syslog message
     * Format: *Mar 1 00:00:00.000: %FACILITY-SEVERITY-MNEMONIC: message
     */
    std::optional<LogEntry> parse_standard_format(const std::string& message) const;

    /**
     * @brief Parse Cisco message with priority
     * Format: <priority>timestamp: %FACILITY-SEVERITY-MNEMONIC: message
     */
    std::optional<LogEntry> parse_priority_format(const std::string& message) const;

    /**
     * @brief Parse simple timestamped message
     * Format: timestamp: message
     */
    std::optional<LogEntry> parse_simple_format(const std::string& message) const;

    /**
     * @brief Extract facility, severity, and mnemonic from Cisco message ID
     * Format: %FACILITY-SEVERITY-MNEMONIC
     */
    struct MessageInfo {
        std::string facility;
        Severity severity;
        std::string mnemonic;
        bool valid = false;
    };
    MessageInfo parse_message_id(const std::string& message_id) const;

    /**
     * @brief Parse Cisco timestamp format
     * Handles various Cisco timestamp formats like:
     * - *Mar 1 00:00:00.000
     * - 00:00:00
     * - Mar 1 2024 00:00:00
     */
    LogEntry::timestamp_t parse_cisco_timestamp(const std::string& timestamp_str) const;

    /**
     * @brief Map Cisco severity number to Severity enum
     */
    Severity map_cisco_severity(int cisco_severity) const;

    // Compiled regex patterns for performance
    std::regex standard_pattern_;
    std::regex priority_pattern_;
    std::regex message_id_pattern_;
    std::regex timestamp_pattern_;
    std::regex simple_timestamp_pattern_;
    
    // Common Cisco log patterns for detection
    std::vector<std::regex> detection_patterns_;
};

} // namespace libnetlog