#include "libnetlog/device_types.hpp"
#include <stdexcept>
#include <algorithm>
#include <cctype>

namespace libnetlog {

std::string to_string(DeviceType device_type) {
    switch (device_type) {
        case DeviceType::Unknown:       return "unknown";
        case DeviceType::CiscoIOS:      return "cisco-ios";
        case DeviceType::CiscoIOSXE:    return "cisco-ios-xe";
        case DeviceType::CiscoNXOS:     return "cisco-nx-os";
        case DeviceType::CiscoASA:      return "cisco-asa";
        case DeviceType::GenericSyslog: return "generic-syslog";
        case DeviceType::Custom:        return "custom";
        default:                        return "unknown";
    }
}

std::string to_string(DeviceVendor vendor) {
    switch (vendor) {
        case DeviceVendor::Unknown: return "unknown";
        case DeviceVendor::Cisco:   return "cisco";
        case DeviceVendor::Juniper: return "juniper";
        case DeviceVendor::Arista:  return "arista";
        case DeviceVendor::HPE:     return "hpe";
        case DeviceVendor::Generic: return "generic";
        default:                    return "unknown";
    }
}

DeviceType parse_device_type(const std::string& type_str) {
    // Convert to lowercase for case-insensitive comparison
    std::string lower_str = type_str;
    std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(), 
                   [](unsigned char c) { return std::tolower(c); });

    if (lower_str == "cisco-ios" || lower_str == "ios") return DeviceType::CiscoIOS;
    if (lower_str == "cisco-ios-xe" || lower_str == "ios-xe") return DeviceType::CiscoIOSXE;
    if (lower_str == "cisco-nx-os" || lower_str == "nxos" || lower_str == "nx-os") return DeviceType::CiscoNXOS;
    if (lower_str == "cisco-asa" || lower_str == "asa") return DeviceType::CiscoASA;
    if (lower_str == "generic-syslog" || lower_str == "syslog") return DeviceType::GenericSyslog;
    if (lower_str == "custom") return DeviceType::Custom;
    if (lower_str == "unknown") return DeviceType::Unknown;

    throw std::invalid_argument("Invalid device type string: " + type_str);
}

DeviceVendor parse_device_vendor(const std::string& vendor_str) {
    // Convert to lowercase for case-insensitive comparison
    std::string lower_str = vendor_str;
    std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(), 
                   [](unsigned char c) { return std::tolower(c); });

    if (lower_str == "cisco") return DeviceVendor::Cisco;
    if (lower_str == "juniper") return DeviceVendor::Juniper;
    if (lower_str == "arista") return DeviceVendor::Arista;
    if (lower_str == "hpe" || lower_str == "hp") return DeviceVendor::HPE;
    if (lower_str == "generic") return DeviceVendor::Generic;
    if (lower_str == "unknown") return DeviceVendor::Unknown;

    throw std::invalid_argument("Invalid device vendor string: " + vendor_str);
}

DeviceType get_default_device_type(DeviceVendor vendor) {
    switch (vendor) {
        case DeviceVendor::Cisco:   return DeviceType::CiscoIOS;
        case DeviceVendor::Generic: return DeviceType::GenericSyslog;
        default:                    return DeviceType::Unknown;
    }
}

} // namespace libnetlog