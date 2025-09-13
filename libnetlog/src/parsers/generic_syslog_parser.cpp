#include "libnetlog/parsers/generic_syslog_parser.hpp"

namespace libnetlog {

GenericSyslogParser::GenericSyslogParser()
    // RFC 3164: <priority>timestamp hostname tag: message
    : rfc3164_pattern_(R"(<(\d+)>(\w+\s+\d+\s+\d+:\d+:\d+)\s+(\S+)\s+(.+?):\s*(.+))")
    // RFC 5424: <priority>version timestamp hostname app-name procid msgid structured-data msg
    , rfc5424_pattern_(R"(<(\d+)>(\d+)\s+(\S+)\s+(\S+)\s+(\S+)\s+(\S+)\s+(\S+)\s+(\S*)\s*(.*))")
    // Simple priority extraction
    , priority_pattern_(R"(<(\d+)>(.*))")
{
}

std::optional<LogEntry> GenericSyslogParser::parse(const std::string& raw_message) {
    if (raw_message.empty()) {
        return std::nullopt;
    }

    std::smatch match;

    // Try RFC 3164 format first
    if (std::regex_search(raw_message, match, rfc3164_pattern_) && match.size() >= 6) {
        int priority = std::stoi(match[1].str());
        std::string timestamp_str = match[2].str();
        std::string hostname = match[3].str();
        std::string tag = match[4].str();
        std::string message = match[5].str();

        // Extract facility and severity from priority
        int facility = priority >> 3;
        int severity_num = priority & 0x07;
        
        Severity severity;
        try {
            severity = parse_severity(static_cast<std::uint8_t>(severity_num));
        } catch (...) {
            severity = Severity::Info;
        }

        // Parse timestamp
        auto timestamp = parse_timestamp(timestamp_str);

        // Create log entry
        auto entry = create_log_entry(timestamp, severity, message, raw_message);
        entry.set_hostname(hostname);
        entry.set_process_name(tag);
        entry.add_metadata("facility_code", std::to_string(facility));
        entry.add_metadata("syslog_priority", std::to_string(priority));
        entry.add_metadata("format", "RFC3164");

        return entry;
    }

    // Try RFC 5424 format
    if (std::regex_search(raw_message, match, rfc5424_pattern_) && match.size() >= 10) {
        int priority = std::stoi(match[1].str());
        std::string version = match[2].str();
        std::string timestamp_str = match[3].str();
        std::string hostname = match[4].str();
        std::string app_name = match[5].str();
        std::string proc_id = match[6].str();
        std::string msg_id = match[7].str();
        std::string structured_data = match[8].str();
        std::string message = match[9].str();

        // Extract facility and severity from priority
        int facility = priority >> 3;
        int severity_num = priority & 0x07;
        
        Severity severity;
        try {
            severity = parse_severity(static_cast<std::uint8_t>(severity_num));
        } catch (...) {
            severity = Severity::Info;
        }

        // Parse timestamp
        auto timestamp = parse_timestamp(timestamp_str);

        // Create log entry
        auto entry = create_log_entry(timestamp, severity, message, raw_message);
        entry.set_hostname(hostname);
        entry.set_process_name(app_name);
        
        if (proc_id != "-") {
            try {
                entry.set_process_id(static_cast<std::uint32_t>(std::stoul(proc_id)));
            } catch (...) {
                // Ignore if process ID is not numeric
            }
        }

        entry.add_metadata("facility_code", std::to_string(facility));
        entry.add_metadata("syslog_priority", std::to_string(priority));
        entry.add_metadata("syslog_version", version);
        entry.add_metadata("message_id", msg_id);
        entry.add_metadata("format", "RFC5424");
        
        if (!structured_data.empty() && structured_data != "-") {
            entry.add_metadata("structured_data", structured_data);
        }

        return entry;
    }

    // Try simple priority extraction as fallback
    if (std::regex_search(raw_message, match, priority_pattern_) && match.size() >= 3) {
        int priority = std::stoi(match[1].str());
        std::string remaining = match[2].str();

        // Extract facility and severity from priority
        int facility = priority >> 3;
        int severity_num = priority & 0x07;
        
        Severity severity;
        try {
            severity = parse_severity(static_cast<std::uint8_t>(severity_num));
        } catch (...) {
            severity = Severity::Info;
        }

        // Use current time since we couldn't parse timestamp
        auto timestamp = std::chrono::system_clock::now();

        // Create log entry
        auto entry = create_log_entry(timestamp, severity, remaining, raw_message);
        entry.add_metadata("facility_code", std::to_string(facility));
        entry.add_metadata("syslog_priority", std::to_string(priority));
        entry.add_metadata("format", "basic_priority");

        return entry;
    }

    return std::nullopt;
}

bool GenericSyslogParser::can_parse(const std::string& raw_message) const {
    // Generic parser can handle most messages with priority
    return std::regex_search(raw_message, priority_pattern_);
}

std::vector<std::string> GenericSyslogParser::get_supported_patterns() const {
    return {
        R"(<\d+>\w+\s+\d+\s+\d+:\d+:\d+\s+\S+\s+.+?:\s*.+)",  // RFC 3164
        R"(<\d+>\d+\s+\S+\s+\S+\s+\S+\s+\S+\s+\S+\s+\S*\s*.*)", // RFC 5424
        R"(<\d+>.*)"  // Basic priority format
    };
}

} // namespace libnetlog