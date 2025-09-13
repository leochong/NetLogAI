#include "libnetlog/parsers/base_parser.hpp"
#include <regex>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace libnetlog {

std::vector<LogEntry> BaseParser::parse_batch(const std::vector<std::string>& raw_messages) {
    std::vector<LogEntry> entries;
    entries.reserve(raw_messages.size());

    for (const auto& raw_message : raw_messages) {
        auto entry = parse(raw_message);
        if (entry) {
            entries.push_back(std::move(*entry));
        }
    }

    return entries;
}

LogEntry BaseParser::create_log_entry(LogEntry::timestamp_t timestamp,
                                     Severity severity,
                                     const std::string& message,
                                     const std::string& raw_message) const {
    LogEntry entry(timestamp, severity, message, get_device_type());
    entry.set_raw_message(raw_message);
    return entry;
}

LogEntry::timestamp_t BaseParser::parse_timestamp(const std::string& timestamp_str) const {
    // Try various timestamp formats common in network device logs
    
    static const std::vector<std::string> timestamp_formats = {
        "%Y-%m-%d %H:%M:%S",           // 2024-01-01 12:00:00
        "%Y-%m-%dT%H:%M:%S",           // 2024-01-01T12:00:00
        "%Y-%m-%dT%H:%M:%SZ",          // 2024-01-01T12:00:00Z
        "%b %d %H:%M:%S",              // Jan 01 12:00:00
        "%b %d %Y %H:%M:%S",           // Jan 01 2024 12:00:00
        "%m/%d/%Y %H:%M:%S",           // 01/01/2024 12:00:00
        "%d/%m/%Y %H:%M:%S",           // 01/01/2024 12:00:00 (European)
    };

    std::tm tm = {};
    std::istringstream ss(timestamp_str);

    // Try each format
    for (const auto& format : timestamp_formats) {
        ss.clear();
        ss.str(timestamp_str);
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

    // If parsing failed, return current time
    return std::chrono::system_clock::now();
}

std::string BaseParser::extract_hostname(const std::string& message) const {
    // Common patterns for hostname extraction
    static const std::vector<std::regex> hostname_patterns = {
        std::regex(R"(^(\w+[\w\.-]*)\s+)"),                    // hostname at start
        std::regex(R"(\s(\w+[\w\.-]*)\s+%\w+)"),              // hostname before facility
        std::regex(R"(<\d+>(\w+[\w\.-]*)\s)"),                // hostname after priority
    };

    for (const auto& pattern : hostname_patterns) {
        std::smatch match;
        if (std::regex_search(message, match, pattern) && match.size() > 1) {
            std::string hostname = match[1].str();
            // Basic validation - hostname shouldn't contain spaces or be too short
            if (hostname.length() > 1 && hostname.find(' ') == std::string::npos) {
                return hostname;
            }
        }
    }

    return "";
}

std::string BaseParser::clean_message(const std::string& message) const {
    std::string cleaned = message;
    
    // Remove leading/trailing whitespace
    auto start = cleaned.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }
    cleaned = cleaned.substr(start);
    
    auto end = cleaned.find_last_not_of(" \t\r\n");
    if (end != std::string::npos) {
        cleaned = cleaned.substr(0, end + 1);
    }
    
    // Remove null characters and other control characters
    cleaned.erase(std::remove_if(cleaned.begin(), cleaned.end(), 
                                [](char c) { return c == '\0' || (c > 0 && c < 32 && c != '\t' && c != '\n'); }), 
                 cleaned.end());
    
    return cleaned;
}

} // namespace libnetlog