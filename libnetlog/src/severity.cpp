#include "libnetlog/severity.hpp"
#include <stdexcept>
#include <algorithm>
#include <cctype>

namespace libnetlog {

std::string to_string(Severity severity) {
    switch (severity) {
        case Severity::Emergency:   return "emergency";
        case Severity::Alert:       return "alert";
        case Severity::Critical:    return "critical";
        case Severity::Error:       return "error";
        case Severity::Warning:     return "warning";
        case Severity::Notice:      return "notice";
        case Severity::Info:        return "info";
        case Severity::Debug:       return "debug";
        default:                    return "unknown";
    }
}

Severity parse_severity(const std::string& severity_str) {
    // Convert to lowercase for case-insensitive comparison
    std::string lower_str = severity_str;
    std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(), 
                   [](unsigned char c) { return std::tolower(c); });

    if (lower_str == "emergency" || lower_str == "emerg") return Severity::Emergency;
    if (lower_str == "alert") return Severity::Alert;
    if (lower_str == "critical" || lower_str == "crit") return Severity::Critical;
    if (lower_str == "error" || lower_str == "err") return Severity::Error;
    if (lower_str == "warning" || lower_str == "warn") return Severity::Warning;
    if (lower_str == "notice" || lower_str == "note") return Severity::Notice;
    if (lower_str == "info" || lower_str == "informational") return Severity::Info;
    if (lower_str == "debug") return Severity::Debug;

    // Try to parse as numeric value
    try {
        int severity_num = std::stoi(severity_str);
        if (severity_num >= 0 && severity_num <= 7) {
            return static_cast<Severity>(severity_num);
        }
    } catch (const std::exception&) {
        // Fall through to throw
    }

    throw std::invalid_argument("Invalid severity string: " + severity_str);
}

Severity parse_severity(std::uint8_t severity_num) {
    if (severity_num > 7) {
        throw std::invalid_argument("Invalid severity number: " + std::to_string(severity_num));
    }
    return static_cast<Severity>(severity_num);
}

} // namespace libnetlog