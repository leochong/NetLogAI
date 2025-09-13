#include "libnetlog/utils/timestamp_parser.hpp"
#include <sstream>
#include <iomanip>
#include <optional>

namespace libnetlog {
namespace utils {

std::vector<std::string> TimestampParser::default_formats_ = {
    "%Y-%m-%d %H:%M:%S",           // 2024-01-01 12:00:00
    "%Y-%m-%dT%H:%M:%S",           // 2024-01-01T12:00:00
    "%Y-%m-%dT%H:%M:%SZ",          // 2024-01-01T12:00:00Z
    "%b %d %H:%M:%S",              // Jan 01 12:00:00
    "%b %d %Y %H:%M:%S",           // Jan 01 2024 12:00:00
    "%m/%d/%Y %H:%M:%S",           // 01/01/2024 12:00:00
    "%d/%m/%Y %H:%M:%S",           // 01/01/2024 12:00:00 (European)
    "%H:%M:%S",                    // 12:00:00 (time only)
};

std::vector<std::string> TimestampParser::custom_formats_;

TimestampParser::timestamp_t TimestampParser::parse(const std::string& timestamp_str) {
    if (timestamp_str.empty()) {
        return std::chrono::system_clock::now();
    }

    // Clean the timestamp string
    std::string clean_str = timestamp_str;
    
    // Remove leading asterisk (common in Cisco logs)
    if (!clean_str.empty() && clean_str[0] == '*') {
        clean_str = clean_str.substr(1);
    }
    
    // Remove milliseconds (.xxx) if present
    size_t dot_pos = clean_str.find('.');
    if (dot_pos != std::string::npos) {
        size_t space_pos = clean_str.find(' ', dot_pos);
        if (space_pos != std::string::npos) {
            clean_str = clean_str.substr(0, dot_pos) + clean_str.substr(space_pos);
        } else {
            clean_str = clean_str.substr(0, dot_pos);
        }
    }

    // Try custom formats first
    for (const auto& format : custom_formats_) {
        auto result = parse_with_format(clean_str, format);
        if (result) {
            return *result;
        }
    }

    // Try default formats
    for (const auto& format : default_formats_) {
        auto result = parse_with_format(clean_str, format);
        if (result) {
            return *result;
        }
    }

    // If all parsing attempts failed, return current time
    return std::chrono::system_clock::now();
}

std::optional<TimestampParser::timestamp_t> TimestampParser::parse_with_format(
    const std::string& timestamp_str, const std::string& format) {
    
    std::tm tm = {};
    std::istringstream ss(timestamp_str);
    ss >> std::get_time(&tm, format.c_str());
    
    if (ss.fail()) {
        return std::nullopt;
    }

    // For formats without year, assume current year
    if (tm.tm_year == 0) {
        auto now = std::chrono::system_clock::now();
        auto now_time_t = std::chrono::system_clock::to_time_t(now);
        auto* now_tm = std::localtime(&now_time_t);
        tm.tm_year = now_tm->tm_year;
    }

    auto time_t_val = std::mktime(&tm);
    if (time_t_val == -1) {
        return std::nullopt;
    }

    return std::chrono::system_clock::from_time_t(time_t_val);
}

void TimestampParser::add_format(const std::string& format) {
    custom_formats_.push_back(format);
}

std::vector<std::string> TimestampParser::get_formats() {
    std::vector<std::string> all_formats;
    all_formats.reserve(custom_formats_.size() + default_formats_.size());
    
    all_formats.insert(all_formats.end(), custom_formats_.begin(), custom_formats_.end());
    all_formats.insert(all_formats.end(), default_formats_.begin(), default_formats_.end());
    
    return all_formats;
}

std::string TimestampParser::to_string(const timestamp_t& timestamp, const std::string& format) {
    auto time_t_val = std::chrono::system_clock::to_time_t(timestamp);
    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&time_t_val), format.c_str());
    return oss.str();
}

} // namespace utils
} // namespace libnetlog