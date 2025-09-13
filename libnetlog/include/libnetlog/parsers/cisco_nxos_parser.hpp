#pragma once

#include "base_parser.hpp"
#include <regex>

namespace libnetlog {

/**
 * @brief Parser for Cisco NX-OS log messages
 * 
 * Handles Cisco NX-OS specific log formats which are similar to IOS
 * but with some differences in timestamp format and facility names.
 */
class CiscoNXOSParser : public BaseParser {
public:
    CiscoNXOSParser();
    ~CiscoNXOSParser() override = default;

    std::optional<LogEntry> parse(const std::string& raw_message) override;
    bool can_parse(const std::string& raw_message) const override;
    DeviceType get_device_type() const override { return DeviceType::CiscoNXOS; }
    std::string get_parser_name() const override { return "Cisco NX-OS Parser"; }
    std::vector<std::string> get_supported_patterns() const override;

private:
    std::vector<std::regex> detection_patterns_;
};

} // namespace libnetlog