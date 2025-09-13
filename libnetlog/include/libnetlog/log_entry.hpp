#pragma once

#include "severity.hpp"
#include "device_types.hpp"
#include <string>
#include <chrono>
#include <optional>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace libnetlog {

/**
 * @brief Represents a single network log entry
 * 
 * This is the core data structure that holds parsed log information
 * from various network devices. It provides a unified interface
 * regardless of the source device type.
 */
class LogEntry {
public:
    using timestamp_t = std::chrono::system_clock::time_point;
    using metadata_t = std::unordered_map<std::string, std::string>;

    /**
     * @brief Default constructor
     */
    LogEntry() = default;

    /**
     * @brief Construct with basic fields
     */
    LogEntry(timestamp_t timestamp, 
             Severity severity, 
             std::string message,
             DeviceType device_type = DeviceType::Unknown);

    /**
     * @brief Construct with full fields
     */
    LogEntry(timestamp_t timestamp,
             Severity severity,
             std::string message,
             std::string facility,
             std::string hostname,
             std::string process_name,
             DeviceType device_type,
             std::optional<std::uint32_t> process_id = std::nullopt);

    // Getters
    const timestamp_t& timestamp() const { return timestamp_; }
    Severity severity() const { return severity_; }
    const std::string& message() const { return message_; }
    const std::string& facility() const { return facility_; }
    const std::string& hostname() const { return hostname_; }
    const std::string& process_name() const { return process_name_; }
    const std::optional<std::uint32_t>& process_id() const { return process_id_; }
    DeviceType device_type() const { return device_type_; }
    const std::string& raw_message() const { return raw_message_; }
    const metadata_t& metadata() const { return metadata_; }

    // Setters
    void set_timestamp(const timestamp_t& timestamp) { timestamp_ = timestamp; }
    void set_severity(Severity severity) { severity_ = severity; }
    void set_message(const std::string& message) { message_ = message; }
    void set_facility(const std::string& facility) { facility_ = facility; }
    void set_hostname(const std::string& hostname) { hostname_ = hostname; }
    void set_process_name(const std::string& process_name) { process_name_ = process_name; }
    void set_process_id(std::uint32_t process_id) { process_id_ = process_id; }
    void set_device_type(DeviceType device_type) { device_type_ = device_type; }
    void set_raw_message(const std::string& raw_message) { raw_message_ = raw_message; }

    // Metadata management
    void add_metadata(const std::string& key, const std::string& value);
    std::optional<std::string> get_metadata(const std::string& key) const;
    bool has_metadata(const std::string& key) const;
    void clear_metadata();

    // Utility methods
    bool is_valid() const;
    std::string to_string() const;
    nlohmann::json to_json() const;
    
    // Static factory methods
    static LogEntry from_json(const nlohmann::json& json);
    static LogEntry from_raw_syslog(const std::string& raw_message, DeviceType device_type = DeviceType::GenericSyslog);

    // Comparison operators
    bool operator==(const LogEntry& other) const;
    bool operator!=(const LogEntry& other) const { return !(*this == other); }

private:
    timestamp_t timestamp_{};
    Severity severity_{Severity::Info};
    std::string message_;
    std::string facility_;
    std::string hostname_;
    std::string process_name_;
    std::optional<std::uint32_t> process_id_;
    DeviceType device_type_{DeviceType::Unknown};
    std::string raw_message_;
    metadata_t metadata_;
};

} // namespace libnetlog