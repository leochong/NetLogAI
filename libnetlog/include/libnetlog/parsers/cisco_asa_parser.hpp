#pragma once

#include "base_parser.hpp"
#include <regex>

namespace libnetlog {

/**
 * @brief Parser for Cisco ASA (Adaptive Security Appliance) log messages
 * 
 * Handles Cisco ASA specific log formats including security events,
 * connection logs, and firewall messages.
 */
class CiscoASAParser : public BaseParser {
public:
    CiscoASAParser();
    ~CiscoASAParser() override = default;

    std::optional<LogEntry> parse(const std::string& raw_message) override;
    bool can_parse(const std::string& raw_message) const override;
    DeviceType get_device_type() const override { return DeviceType::CiscoASA; }
    std::string get_parser_name() const override { return "Cisco ASA Parser"; }
    std::vector<std::string> get_supported_patterns() const override;

private:
    std::vector<std::regex> detection_patterns_;
};

} // namespace libnetlog