#include "analytics/git_style_commands.hpp"
#include <regex>
#include <algorithm>
#include <unordered_map>
#include <cmath>
#include <iostream>

namespace netlogai::analytics {

PatternAnalyzer::PatternAnalyzer() {
    load_default_patterns();
}

void PatternAnalyzer::load_default_patterns() {
    // Network interface patterns
    patterns_.push_back({
        "interface_down",
        R"(%LINEPROTO-5-UPDOWN:\s+Line\s+protocol\s+on\s+Interface\s+([A-Za-z0-9\/]+),\s+changed\s+state\s+to\s+down)",
        "Interface going down - potential connectivity issue",
        8,
        {"interface_up", "physical_layer"}
    });

    patterns_.push_back({
        "interface_up",
        R"(%LINEPROTO-5-UPDOWN:\s+Line\s+protocol\s+on\s+Interface\s+([A-Za-z0-9\/]+),\s+changed\s+state\s+to\s+up)",
        "Interface coming up - connectivity restored",
        3,
        {"interface_down", "physical_layer"}
    });

    // BGP patterns
    patterns_.push_back({
        "bgp_session_down",
        R"(%BGP-5-ADJCHANGE:\s+neighbor\s+([0-9\.]+)\s+Down)",
        "BGP neighbor session down - routing impact",
        9,
        {"bgp_session_up", "routing_protocol"}
    });

    patterns_.push_back({
        "bgp_session_up",
        R"(%BGP-5-ADJCHANGE:\s+neighbor\s+([0-9\.]+)\s+Up)",
        "BGP neighbor session established",
        4,
        {"bgp_session_down", "routing_protocol"}
    });

    // OSPF patterns
    patterns_.push_back({
        "ospf_neighbor_down",
        R"(%OSPF-5-ADJCHG:\s+Process\s+\d+,\s+Nbr\s+([0-9\.]+)\s+on\s+([A-Za-z0-9\/]+)\s+from\s+\w+\s+to\s+Down)",
        "OSPF neighbor down - routing convergence",
        8,
        {"ospf_neighbor_up", "routing_protocol"}
    });

    // Authentication patterns
    patterns_.push_back({
        "login_failure",
        R"(Login\s+invalid|Authentication\s+failed|Invalid\s+username)",
        "Authentication failure - potential security concern",
        7,
        {"security_event", "access_control"}
    });

    // Hardware patterns
    patterns_.push_back({
        "temperature_high",
        R"(Temperature\s+(?:sensor|warning|critical)|Thermal\s+(?:warning|shutdown))",
        "High temperature warning - hardware concern",
        6,
        {"hardware_event", "environmental"}
    });

    // Power patterns
    patterns_.push_back({
        "power_supply_fail",
        R"(Power\s+supply\s+(?:failure|fail|down)|PSU\s+(?:failure|fail))",
        "Power supply failure - critical hardware issue",
        10,
        {"hardware_event", "power"}
    });

    // CPU/Memory patterns
    patterns_.push_back({
        "high_cpu",
        R"(CPU\s+utilization\s+(?:high|above|over)\s+(\d+)%)",
        "High CPU utilization detected",
        7,
        {"performance_issue", "resource"}
    });

    patterns_.push_back({
        "memory_low",
        R"(Memory\s+(?:low|insufficient|critical)|Out\s+of\s+memory)",
        "Low memory condition",
        8,
        {"performance_issue", "resource"}
    });
}

std::vector<PatternAnalyzer::PatternMatch> PatternAnalyzer::analyze_patterns(
    const std::vector<LogEntry>& entries) {

    std::vector<PatternMatch> matches;

    for (const auto& entry : entries) {
        for (const auto& pattern : patterns_) {
            std::regex regex_pattern(pattern.regex, std::regex_constants::icase);
            std::smatch match_result;

            if (std::regex_search(entry.message, match_result, regex_pattern) ||
                std::regex_search(entry.raw_line, match_result, regex_pattern)) {

                PatternMatch match;
                match.pattern = pattern;
                match.entry = entry;

                // Calculate confidence based on regex match quality
                match.confidence = 0.8; // Base confidence
                if (match_result.size() > 1) {
                    match.confidence += 0.15; // Bonus for captured groups
                }

                // Extract captured groups
                for (size_t i = 1; i < match_result.size(); ++i) {
                    match.extracted_values.push_back(match_result[i].str());
                }

                matches.push_back(match);
            }
        }
    }

    // Sort by severity weight (descending) then by confidence
    std::sort(matches.begin(), matches.end(), [](const PatternMatch& a, const PatternMatch& b) {
        if (a.pattern.severity_weight != b.pattern.severity_weight) {
            return a.pattern.severity_weight > b.pattern.severity_weight;
        }
        return a.confidence > b.confidence;
    });

    return matches;
}

std::vector<PatternAnalyzer::PatternMatch> PatternAnalyzer::find_pattern(
    const std::string& pattern_name, const std::vector<LogEntry>& entries) {

    auto pattern_it = std::find_if(patterns_.begin(), patterns_.end(),
        [&pattern_name](const Pattern& p) { return p.name == pattern_name; });

    if (pattern_it == patterns_.end()) {
        return {};
    }

    std::vector<PatternMatch> matches;
    std::regex regex_pattern(pattern_it->regex, std::regex_constants::icase);

    for (const auto& entry : entries) {
        std::smatch match_result;
        if (std::regex_search(entry.message, match_result, regex_pattern) ||
            std::regex_search(entry.raw_line, match_result, regex_pattern)) {

            PatternMatch match;
            match.pattern = *pattern_it;
            match.entry = entry;
            match.confidence = 0.9; // High confidence for specific pattern search

            for (size_t i = 1; i < match_result.size(); ++i) {
                match.extracted_values.push_back(match_result[i].str());
            }

            matches.push_back(match);
        }
    }

    return matches;
}

std::vector<LogEntry> PatternAnalyzer::find_correlations(
    const std::vector<LogEntry>& entries, int time_window_minutes) {

    std::vector<LogEntry> correlated;
    auto time_window = std::chrono::minutes(time_window_minutes);

    // Find patterns that occurred within time window
    for (size_t i = 0; i < entries.size(); ++i) {
        for (size_t j = i + 1; j < entries.size(); ++j) {
            auto time_diff = entries[j].timestamp - entries[i].timestamp;

            if (time_diff <= time_window) {
                // Check if events are potentially related
                bool potentially_related = false;

                // Same device
                if (entries[i].device_name == entries[j].device_name) {
                    potentially_related = true;
                }

                // Same interface
                if (!entries[i].interface.empty() &&
                    entries[i].interface == entries[j].interface) {
                    potentially_related = true;
                }

                // Related severity levels (error followed by warning/info)
                if ((entries[i].severity == "error" &&
                     (entries[j].severity == "warning" || entries[j].severity == "info")) ||
                    (entries[i].severity == "warning" && entries[j].severity == "info")) {
                    potentially_related = true;
                }

                if (potentially_related) {
                    // Add both entries if not already present
                    if (std::find_if(correlated.begin(), correlated.end(),
                        [&](const LogEntry& e) { return e.id == entries[i].id; }) == correlated.end()) {
                        correlated.push_back(entries[i]);
                    }
                    if (std::find_if(correlated.begin(), correlated.end(),
                        [&](const LogEntry& e) { return e.id == entries[j].id; }) == correlated.end()) {
                        correlated.push_back(entries[j]);
                    }
                }
            } else {
                break; // Entries are chronologically sorted
            }
        }
    }

    return correlated;
}

std::vector<LogEntry> PatternAnalyzer::detect_anomalies(const std::vector<LogEntry>& entries) {
    std::vector<LogEntry> anomalies;
    std::unordered_map<std::string, int> message_frequency;
    std::unordered_map<std::string, int> device_frequency;

    // Build frequency maps
    for (const auto& entry : entries) {
        message_frequency[entry.message]++;
        device_frequency[entry.device_name]++;
    }

    // Calculate thresholds
    double total_entries = static_cast<double>(entries.size());

    for (const auto& entry : entries) {
        bool is_anomaly = false;

        // Rare message (appears in less than 1% of logs)
        if (message_frequency[entry.message] / total_entries < 0.01) {
            is_anomaly = true;
        }

        // Critical severity
        if (entry.severity == "critical" || entry.severity == "emergency") {
            is_anomaly = true;
        }

        // Unusual time patterns (outside business hours with high severity)
        auto time_t = std::chrono::system_clock::to_time_t(entry.timestamp);
        auto* tm = std::localtime(&time_t);
        if ((tm->tm_hour < 6 || tm->tm_hour > 22) &&
            (entry.severity == "error" || entry.severity == "critical")) {
            is_anomaly = true;
        }

        if (is_anomaly) {
            anomalies.push_back(entry);
        }
    }

    return anomalies;
}

std::map<std::string, int> PatternAnalyzer::generate_statistics(const std::vector<LogEntry>& entries) {
    std::map<std::string, int> stats;

    // Severity distribution
    std::unordered_map<std::string, int> severity_count;
    std::unordered_map<std::string, int> device_count;
    std::unordered_map<std::string, int> interface_count;

    for (const auto& entry : entries) {
        severity_count[entry.severity]++;
        device_count[entry.device_name]++;
        if (!entry.interface.empty()) {
            interface_count[entry.interface]++;
        }
    }

    // Convert to output format
    for (const auto& [severity, count] : severity_count) {
        stats["severity_" + severity] = count;
    }

    stats["total_entries"] = static_cast<int>(entries.size());
    stats["unique_devices"] = static_cast<int>(device_count.size());
    stats["unique_interfaces"] = static_cast<int>(interface_count.size());

    // Pattern detection statistics
    auto pattern_matches = analyze_patterns(entries);
    stats["pattern_matches"] = static_cast<int>(pattern_matches.size());

    std::unordered_map<std::string, int> pattern_type_count;
    for (const auto& match : pattern_matches) {
        pattern_type_count[match.pattern.name]++;
    }

    for (const auto& [pattern, count] : pattern_type_count) {
        stats["pattern_" + pattern] = count;
    }

    return stats;
}

} // namespace netlogai::analytics