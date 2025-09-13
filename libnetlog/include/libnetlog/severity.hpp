#pragma once

#include <string>
#include <cstdint>

namespace libnetlog {

/**
 * @brief Standard syslog severity levels (RFC 3164)
 */
enum class Severity : std::uint8_t {
    Emergency   = 0,  // System is unusable
    Alert       = 1,  // Action must be taken immediately
    Critical    = 2,  // Critical conditions
    Error       = 3,  // Error conditions
    Warning     = 4,  // Warning conditions
    Notice      = 5,  // Normal but significant condition
    Info        = 6,  // Informational messages
    Debug       = 7   // Debug-level messages
};

/**
 * @brief Convert severity enum to string
 */
std::string to_string(Severity severity);

/**
 * @brief Parse severity from string
 */
Severity parse_severity(const std::string& severity_str);

/**
 * @brief Parse severity from numeric value
 */
Severity parse_severity(std::uint8_t severity_num);

} // namespace libnetlog