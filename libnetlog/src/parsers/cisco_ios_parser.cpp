#include "libnetlog/parsers/cisco_ios_parser.hpp"
#include <sstream>
#include <iomanip>
#include <map>

namespace libnetlog {

CiscoIOSParser::CiscoIOSParser() 
    // Standard Cisco IOS format: *Mar 1 00:00:00.000: %FACILITY-SEVERITY-MNEMONIC: message
    : standard_pattern_(R"(\*?(\w+\s+\d+\s+\d+:\d+:\d+(?:\.\d+)?)\s*:\s*%([A-Z_]+)-(\d+)-([A-Z_]+):\s*(.+))")
    // Priority format: <priority>timestamp: %FACILITY-SEVERITY-MNEMONIC: message
    , priority_pattern_(R"(<(\d+)>(.+?):\s*%([A-Z_]+)-(\d+)-([A-Z_]+):\s*(.+))")
    // Message ID pattern: %FACILITY-SEVERITY-MNEMONIC
    , message_id_pattern_(R"(%([A-Z_]+)-(\d+)-([A-Z_]+))")
    // Cisco timestamp patterns
    , timestamp_pattern_(R"(\*?(\w+\s+\d+\s+\d+:\d+:\d+(?:\.\d+)?|\d+:\d+:\d+(?:\.\d+)?|\w+\s+\d+\s+\d+\s+\d+:\d+:\d+))")
    , simple_timestamp_pattern_(R"((\d+:\d+:\d+(?:\.\d+)?))")
{
    // Initialize detection patterns
    detection_patterns_ = {
        std::regex(R"(%[A-Z_]+-\d+-[A-Z_]+:)"),           // Cisco message format
        std::regex(R"(\*\w+\s+\d+\s+\d+:\d+:\d+)"),       // Cisco timestamp
        std::regex(R"(%LINEPROTO-|%LINK-|%BGP-|%OSPF-)"),  // Common Cisco facilities
        std::regex(R"(%SYS-|%CONFIG_I-|%SEC-)"),           // More Cisco facilities
    };
}

std::optional<LogEntry> CiscoIOSParser::parse(const std::string& raw_message) {
    if (raw_message.empty()) {
        return std::nullopt;
    }

    // Try standard format first
    auto result = parse_standard_format(raw_message);
    if (result) {
        return result;
    }

    // Try priority format
    result = parse_priority_format(raw_message);
    if (result) {
        return result;
    }

    // Try simple format as fallback
    result = parse_simple_format(raw_message);
    if (result) {
        return result;
    }

    return std::nullopt;
}

bool CiscoIOSParser::can_parse(const std::string& raw_message) const {
    // Check if any of our detection patterns match
    for (const auto& pattern : detection_patterns_) {
        if (std::regex_search(raw_message, pattern)) {
            return true;
        }
    }
    return false;
}

std::vector<std::string> CiscoIOSParser::get_supported_patterns() const {
    return {
        R"(\*\w+\s+\d+\s+\d+:\d+:\d+(?:\.\d+)?\s*:\s*%[A-Z_]+-\d+-[A-Z_]+:.*)",
        R"(<\d+>.+?:\s*%[A-Z_]+-\d+-[A-Z_]+:.*)",
        R"(\d+:\d+:\d+(?:\.\d+)?\s*:\s*%[A-Z_]+-\d+-[A-Z_]+:.*)"
    };
}

std::optional<LogEntry> CiscoIOSParser::parse_standard_format(const std::string& message) const {
    std::smatch match;
    if (std::regex_search(message, match, standard_pattern_) && match.size() >= 6) {
        std::string timestamp_str = match[1].str();
        std::string facility = match[2].str();
        std::string severity_str = match[3].str();
        std::string mnemonic = match[4].str();
        std::string msg_content = match[5].str();

        // Parse timestamp
        auto timestamp = parse_cisco_timestamp(timestamp_str);

        // Parse severity
        int cisco_severity = std::stoi(severity_str);
        auto severity = map_cisco_severity(cisco_severity);

        // Create log entry
        auto entry = create_log_entry(timestamp, severity, msg_content, message);
        entry.set_facility(facility);
        entry.add_metadata("mnemonic", mnemonic);
        entry.add_metadata("cisco_severity", severity_str);

        // Try to extract hostname
        std::string hostname = extract_hostname(message);
        if (!hostname.empty()) {
            entry.set_hostname(hostname);
        }

        return entry;
    }

    return std::nullopt;
}

std::optional<LogEntry> CiscoIOSParser::parse_priority_format(const std::string& message) const {
    std::smatch match;
    if (std::regex_search(message, match, priority_pattern_) && match.size() >= 7) {
        std::string priority_str = match[1].str();
        std::string timestamp_str = match[2].str();
        std::string facility = match[3].str();
        std::string severity_str = match[4].str();
        std::string mnemonic = match[5].str();
        std::string msg_content = match[6].str();

        // Parse timestamp
        auto timestamp = parse_cisco_timestamp(timestamp_str);

        // Parse severity (prefer Cisco severity over syslog priority)
        int cisco_severity = std::stoi(severity_str);
        auto severity = map_cisco_severity(cisco_severity);

        // Create log entry
        auto entry = create_log_entry(timestamp, severity, msg_content, message);
        entry.set_facility(facility);
        entry.add_metadata("mnemonic", mnemonic);
        entry.add_metadata("cisco_severity", severity_str);
        entry.add_metadata("syslog_priority", priority_str);

        // Try to extract hostname
        std::string hostname = extract_hostname(message);
        if (!hostname.empty()) {
            entry.set_hostname(hostname);
        }

        return entry;
    }

    return std::nullopt;
}

std::optional<LogEntry> CiscoIOSParser::parse_simple_format(const std::string& message) const {
    // Look for message ID pattern anywhere in the message
    std::smatch msg_match;
    if (std::regex_search(message, msg_match, message_id_pattern_) && msg_match.size() >= 4) {
        std::string facility = msg_match[1].str();
        std::string severity_str = msg_match[2].str();
        std::string mnemonic = msg_match[3].str();

        // Try to find timestamp at the beginning
        std::smatch ts_match;
        LogEntry::timestamp_t timestamp;
        if (std::regex_search(message, ts_match, timestamp_pattern_)) {
            timestamp = parse_cisco_timestamp(ts_match[1].str());
        } else {
            timestamp = std::chrono::system_clock::now();
        }

        // Parse severity
        int cisco_severity = std::stoi(severity_str);
        auto severity = map_cisco_severity(cisco_severity);

        // Extract message content (everything after the message ID)
        std::string msg_content = message;
        size_t msg_id_pos = message.find('%' + facility + '-' + severity_str + '-' + mnemonic + ':');
        if (msg_id_pos != std::string::npos) {
            size_t content_start = msg_id_pos + facility.length() + severity_str.length() + mnemonic.length() + 3; // +3 for %--:
            if (content_start < message.length()) {
                msg_content = message.substr(content_start);
                msg_content = clean_message(msg_content);
            }
        }

        // Create log entry
        auto entry = create_log_entry(timestamp, severity, msg_content, message);
        entry.set_facility(facility);
        entry.add_metadata("mnemonic", mnemonic);
        entry.add_metadata("cisco_severity", severity_str);

        // Try to extract hostname
        std::string hostname = extract_hostname(message);
        if (!hostname.empty()) {
            entry.set_hostname(hostname);
        }

        return entry;
    }

    return std::nullopt;
}

LogEntry::timestamp_t CiscoIOSParser::parse_cisco_timestamp(const std::string& timestamp_str) const {
    // Common Cisco timestamp formats:
    // *Mar 1 00:00:00.000
    // Mar 1 2024 00:00:00
    // 00:00:00.000
    
    std::tm tm = {};
    std::istringstream ss(timestamp_str);
    
    // Try different formats
    static const std::vector<std::pair<std::string, std::string>> cisco_formats = {
        {"*%b %d %H:%M:%S", "*Mar 1 00:00:00"},           // *Mar 1 00:00:00.000
        {"%b %d %Y %H:%M:%S", "Mar 1 2024 00:00:00"},     // Mar 1 2024 00:00:00
        {"%b %d %H:%M:%S", "Mar 1 00:00:00"},             // Mar 1 00:00:00
        {"%H:%M:%S", "00:00:00"},                         // 00:00:00.000
    };

    for (const auto& [format, example] : cisco_formats) {
        ss.clear();
        ss.str(timestamp_str);
        
        // Remove asterisk if present
        std::string clean_ts = timestamp_str;
        if (!clean_ts.empty() && clean_ts[0] == '*') {
            clean_ts = clean_ts.substr(1);
            ss.str(clean_ts);
        }
        
        // Remove milliseconds if present
        size_t dot_pos = clean_ts.find('.');
        if (dot_pos != std::string::npos) {
            clean_ts = clean_ts.substr(0, dot_pos);
            ss.str(clean_ts);
        }

        ss >> std::get_time(&tm, format.c_str());
        
        if (!ss.fail()) {
            // For formats without year, assume current year
            if (tm.tm_year == 0) {
                auto now = std::chrono::system_clock::now();
                auto now_time_t = std::chrono::system_clock::to_time_t(now);
                auto* now_tm = std::localtime(&now_time_t);
                tm.tm_year = now_tm->tm_year;
            }
            
            auto time_t_val = std::mktime(&tm);
            if (time_t_val != -1) {
                return std::chrono::system_clock::from_time_t(time_t_val);
            }
        }
    }

    // If all parsing attempts failed, return current time
    return std::chrono::system_clock::now();
}

Severity CiscoIOSParser::map_cisco_severity(int cisco_severity) const {
    // Cisco uses 0-7 scale similar to syslog, but with specific meanings
    static const std::map<int, Severity> severity_map = {
        {0, Severity::Emergency},   // System unusable
        {1, Severity::Alert},       // Immediate action required
        {2, Severity::Critical},    // Critical condition
        {3, Severity::Error},       // Error condition
        {4, Severity::Warning},     // Warning condition
        {5, Severity::Notice},      // Normal but significant
        {6, Severity::Info},        // Informational
        {7, Severity::Debug}        // Debug messages
    };

    auto it = severity_map.find(cisco_severity);
    if (it != severity_map.end()) {
        return it->second;
    }

    // Default to Info for unknown severities
    return Severity::Info;
}

} // namespace libnetlog