#pragma once

#include "parsers/base_parser.hpp"
#include "device_types.hpp"
#include <memory>
#include <unordered_map>
#include <vector>
#include <functional>

namespace libnetlog {

/**
 * @brief Factory class for creating and managing log parsers
 * 
 * This class provides a centralized way to create parser instances
 * based on device type or by auto-detecting the appropriate parser
 * for a given log message.
 */
class ParserFactory {
public:
    using ParserPtr = std::unique_ptr<BaseParser>;
    using ParserCreator = std::function<ParserPtr()>;

    /**
     * @brief Get the singleton instance of ParserFactory
     */
    static ParserFactory& instance();

    /**
     * @brief Create a parser for a specific device type
     * 
     * @param device_type The type of device to create a parser for
     * @return Unique pointer to parser, or nullptr if not supported
     */
    ParserPtr create_parser(DeviceType device_type);

    /**
     * @brief Auto-detect and create appropriate parser for a log message
     * 
     * @param raw_message Sample log message to analyze
     * @return Unique pointer to best matching parser, or nullptr if none found
     */
    ParserPtr auto_detect_parser(const std::string& raw_message);

    /**
     * @brief Register a custom parser creator function
     * 
     * @param device_type Device type this parser handles
     * @param creator Function that creates parser instances
     * @return true if registered successfully, false if device_type already registered
     */
    bool register_parser(DeviceType device_type, ParserCreator creator);

    /**
     * @brief Unregister a parser for a device type
     * 
     * @param device_type Device type to unregister
     * @return true if unregistered successfully, false if not found
     */
    bool unregister_parser(DeviceType device_type);

    /**
     * @brief Get list of supported device types
     * 
     * @return Vector of supported DeviceType values
     */
    std::vector<DeviceType> get_supported_device_types() const;

    /**
     * @brief Check if a device type is supported
     * 
     * @param device_type Device type to check
     * @return true if supported, false otherwise
     */
    bool is_supported(DeviceType device_type) const;

    /**
     * @brief Get information about all registered parsers
     * 
     * @return Vector of parser information (name, version, device type)
     */
    struct ParserInfo {
        std::string name;
        std::string version;
        DeviceType device_type;
        std::vector<std::string> supported_patterns;
    };
    std::vector<ParserInfo> get_parser_info() const;

private:
    ParserFactory();
    ~ParserFactory() = default;
    
    // Delete copy constructor and assignment operator
    ParserFactory(const ParserFactory&) = delete;
    ParserFactory& operator=(const ParserFactory&) = delete;

    /**
     * @brief Register built-in parsers
     */
    void register_builtin_parsers();

    std::unordered_map<DeviceType, ParserCreator> parsers_;
};

} // namespace libnetlog