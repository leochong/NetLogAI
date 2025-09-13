#pragma once

#include <string>
#include <chrono>
#include <vector>

namespace libnetlog {
namespace utils {

/**
 * @brief Utility class for parsing various timestamp formats
 * 
 * This class provides methods to parse timestamps from different
 * network devices that may use various date/time formats.
 */
class TimestampParser {
public:
    using timestamp_t = std::chrono::system_clock::time_point;

    /**
     * @brief Parse timestamp from string using multiple format attempts
     * 
     * @param timestamp_str The timestamp string to parse
     * @return Parsed timestamp or current time if parsing fails
     */
    static timestamp_t parse(const std::string& timestamp_str);

    /**
     * @brief Parse timestamp with a specific format
     * 
     * @param timestamp_str The timestamp string to parse
     * @param format The strptime format string
     * @return Parsed timestamp or nullopt if parsing fails
     */
    static std::optional<timestamp_t> parse_with_format(const std::string& timestamp_str, 
                                                       const std::string& format);

    /**
     * @brief Add a custom timestamp format to try during parsing
     * 
     * @param format The strptime format string to add
     */
    static void add_format(const std::string& format);

    /**
     * @brief Get all registered timestamp formats
     * 
     * @return Vector of format strings
     */
    static std::vector<std::string> get_formats();

    /**
     * @brief Convert timestamp to string
     * 
     * @param timestamp The timestamp to convert
     * @param format The strftime format string (default: ISO 8601)
     * @return Formatted timestamp string
     */
    static std::string to_string(const timestamp_t& timestamp, 
                                const std::string& format = "%Y-%m-%dT%H:%M:%SZ");

private:
    static std::vector<std::string> default_formats_;
    static std::vector<std::string> custom_formats_;
};

} // namespace utils
} // namespace libnetlog