#include "libnetlog/log_entry.hpp"
#include <sstream>
#include <iomanip>

namespace libnetlog {

LogEntry::LogEntry(timestamp_t timestamp, 
                   Severity severity, 
                   std::string message,
                   DeviceType device_type)
    : timestamp_(timestamp)
    , severity_(severity)
    , message_(std::move(message))
    , device_type_(device_type) {
}

LogEntry::LogEntry(timestamp_t timestamp,
                   Severity severity,
                   std::string message,
                   std::string facility,
                   std::string hostname,
                   std::string process_name,
                   DeviceType device_type,
                   std::optional<std::uint32_t> process_id)
    : timestamp_(timestamp)
    , severity_(severity)
    , message_(std::move(message))
    , facility_(std::move(facility))
    , hostname_(std::move(hostname))
    , process_name_(std::move(process_name))
    , process_id_(process_id)
    , device_type_(device_type) {
}

void LogEntry::add_metadata(const std::string& key, const std::string& value) {
    metadata_[key] = value;
}

std::optional<std::string> LogEntry::get_metadata(const std::string& key) const {
    auto it = metadata_.find(key);
    if (it != metadata_.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool LogEntry::has_metadata(const std::string& key) const {
    return metadata_.find(key) != metadata_.end();
}

void LogEntry::clear_metadata() {
    metadata_.clear();
}

bool LogEntry::is_valid() const {
    // A valid log entry must have at least a timestamp and a message
    return !message_.empty();
}

std::string LogEntry::to_string() const {
    std::ostringstream oss;
    
    // Format timestamp
    auto time_t = std::chrono::system_clock::to_time_t(timestamp_);
    oss << std::put_time(std::gmtime(&time_t), "%Y-%m-%d %H:%M:%S UTC");
    
    // Add severity
    oss << " [" << libnetlog::to_string(severity_) << "]";
    
    // Add hostname if available
    if (!hostname_.empty()) {
        oss << " " << hostname_;
    }
    
    // Add facility and process info if available
    if (!facility_.empty()) {
        oss << " " << facility_;
        if (!process_name_.empty()) {
            oss << "[" << process_name_;
            if (process_id_) {
                oss << ":" << *process_id_;
            }
            oss << "]";
        }
    }
    
    // Add the main message
    oss << ": " << message_;
    
    return oss.str();
}

nlohmann::json LogEntry::to_json() const {
    nlohmann::json j;
    
    // Convert timestamp to ISO 8601 string
    auto time_t = std::chrono::system_clock::to_time_t(timestamp_);
    std::ostringstream timestamp_str;
    timestamp_str << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
    
    j["timestamp"] = timestamp_str.str();
    j["severity"] = libnetlog::to_string(severity_);
    j["message"] = message_;
    j["device_type"] = libnetlog::to_string(device_type_);
    
    if (!facility_.empty()) j["facility"] = facility_;
    if (!hostname_.empty()) j["hostname"] = hostname_;
    if (!process_name_.empty()) j["process_name"] = process_name_;
    if (process_id_) j["process_id"] = *process_id_;
    if (!raw_message_.empty()) j["raw_message"] = raw_message_;
    
    if (!metadata_.empty()) {
        j["metadata"] = metadata_;
    }
    
    return j;
}

LogEntry LogEntry::from_json(const nlohmann::json& json) {
    LogEntry entry;
    
    // Parse timestamp
    if (json.contains("timestamp")) {
        std::string timestamp_str = json["timestamp"];
        std::istringstream iss(timestamp_str);
        std::tm tm = {};
        iss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
        if (!iss.fail()) {
            entry.timestamp_ = std::chrono::system_clock::from_time_t(std::mktime(&tm));
        }
    }
    
    // Parse severity
    if (json.contains("severity")) {
        try {
            entry.severity_ = parse_severity(json["severity"].get<std::string>());
        } catch (const std::exception&) {
            entry.severity_ = Severity::Info;
        }
    }
    
    // Parse device type
    if (json.contains("device_type")) {
        try {
            entry.device_type_ = parse_device_type(json["device_type"].get<std::string>());
        } catch (const std::exception&) {
            entry.device_type_ = DeviceType::Unknown;
        }
    }
    
    // Parse other fields
    if (json.contains("message")) entry.message_ = json["message"];
    if (json.contains("facility")) entry.facility_ = json["facility"];
    if (json.contains("hostname")) entry.hostname_ = json["hostname"];
    if (json.contains("process_name")) entry.process_name_ = json["process_name"];
    if (json.contains("process_id")) entry.process_id_ = json["process_id"];
    if (json.contains("raw_message")) entry.raw_message_ = json["raw_message"];
    
    // Parse metadata
    if (json.contains("metadata") && json["metadata"].is_object()) {
        entry.metadata_ = json["metadata"];
    }
    
    return entry;
}

LogEntry LogEntry::from_raw_syslog(const std::string& raw_message, DeviceType device_type) {
    LogEntry entry;
    entry.raw_message_ = raw_message;
    entry.device_type_ = device_type;
    entry.timestamp_ = std::chrono::system_clock::now();
    entry.severity_ = Severity::Info;
    entry.message_ = raw_message; // Will be parsed by specific parsers
    
    return entry;
}

bool LogEntry::operator==(const LogEntry& other) const {
    return timestamp_ == other.timestamp_ &&
           severity_ == other.severity_ &&
           message_ == other.message_ &&
           facility_ == other.facility_ &&
           hostname_ == other.hostname_ &&
           process_name_ == other.process_name_ &&
           process_id_ == other.process_id_ &&
           device_type_ == other.device_type_ &&
           raw_message_ == other.raw_message_ &&
           metadata_ == other.metadata_;
}

} // namespace libnetlog