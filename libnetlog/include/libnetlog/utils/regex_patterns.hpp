#pragma once

#include <string>
#include <regex>
#include <vector>

namespace libnetlog {
namespace utils {

/**
 * @brief Collection of common regex patterns used for log parsing
 * 
 * This class provides pre-compiled regex patterns that are commonly
 * used across different parsers for extracting information from logs.
 */
class RegexPatterns {
public:
    // Timestamp patterns
    static const std::regex ISO8601_TIMESTAMP;
    static const std::regex SYSLOG_TIMESTAMP;
    static const std::regex CISCO_TIMESTAMP;
    static const std::regex SIMPLE_TIME;

    // Network patterns
    static const std::regex IPV4_ADDRESS;
    static const std::regex IPV6_ADDRESS;
    static const std::regex MAC_ADDRESS;
    static const std::regex INTERFACE_NAME;

    // Syslog patterns
    static const std::regex SYSLOG_PRIORITY;
    static const std::regex SYSLOG_RFC3164;
    static const std::regex SYSLOG_RFC5424;

    // Cisco specific patterns
    static const std::regex CISCO_MESSAGE_ID;
    static const std::regex CISCO_FACILITY;

    // Common log elements
    static const std::regex HOSTNAME;
    static const std::regex PROCESS_NAME;
    static const std::regex SEVERITY_WORD;

    /**
     * @brief Test if a string matches an IPv4 address pattern
     */
    static bool is_ipv4(const std::string& str);

    /**
     * @brief Test if a string matches an IPv6 address pattern
     */
    static bool is_ipv6(const std::string& str);

    /**
     * @brief Test if a string matches a MAC address pattern
     */
    static bool is_mac_address(const std::string& str);

    /**
     * @brief Extract all IPv4 addresses from a string
     */
    static std::vector<std::string> extract_ipv4_addresses(const std::string& str);

    /**
     * @brief Extract all IPv6 addresses from a string
     */
    static std::vector<std::string> extract_ipv6_addresses(const std::string& str);

    /**
     * @brief Extract interface names from a string
     */
    static std::vector<std::string> extract_interface_names(const std::string& str);
};

} // namespace utils
} // namespace libnetlog