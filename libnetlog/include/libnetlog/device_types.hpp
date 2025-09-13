#pragma once

#include <string>
#include <cstdint>

namespace libnetlog {

/**
 * @brief Supported network device types
 */
enum class DeviceType : std::uint8_t {
    Unknown = 0,
    CiscoIOS,
    CiscoIOSXE,
    CiscoNXOS,
    CiscoASA,
    GenericSyslog,
    Custom
};

/**
 * @brief Device vendor identification
 */
enum class DeviceVendor : std::uint8_t {
    Unknown = 0,
    Cisco,
    Juniper,
    Arista,
    HPE,
    Generic
};

/**
 * @brief Convert device type enum to string
 */
std::string to_string(DeviceType device_type);

/**
 * @brief Convert device vendor enum to string
 */
std::string to_string(DeviceVendor vendor);

/**
 * @brief Parse device type from string
 */
DeviceType parse_device_type(const std::string& type_str);

/**
 * @brief Parse device vendor from string
 */
DeviceVendor parse_device_vendor(const std::string& vendor_str);

/**
 * @brief Get default device type for a vendor
 */
DeviceType get_default_device_type(DeviceVendor vendor);

} // namespace libnetlog