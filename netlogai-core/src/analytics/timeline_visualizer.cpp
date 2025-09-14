#include "analytics/git_style_commands.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <unordered_map>
#include <ctime>

namespace netlogai::analytics {

TimelineVisualizer::TimelineVisualizer() {}

std::vector<TimelineVisualizer::TimelineEvent> TimelineVisualizer::create_timeline(
    const std::vector<LogEntry>& entries) {

    std::vector<TimelineEvent> events;

    // Group entries by time windows (5-minute intervals)
    std::unordered_map<int64_t, std::vector<LogEntry>> time_buckets;

    for (const auto& entry : entries) {
        // Round to 5-minute intervals
        auto timestamp_seconds = std::chrono::duration_cast<std::chrono::seconds>(
            entry.timestamp.time_since_epoch()).count();
        int64_t bucket = (timestamp_seconds / 300) * 300; // 300 seconds = 5 minutes
        time_buckets[bucket].push_back(entry);
    }

    // Convert buckets to timeline events
    for (const auto& [bucket_time, bucket_entries] : time_buckets) {
        TimelineEvent event;
        event.timestamp = std::chrono::system_clock::from_time_t(bucket_time);
        event.related_entries = bucket_entries;

        // Determine primary device and event type
        std::unordered_map<std::string, int> device_count;
        std::unordered_map<std::string, int> severity_count;

        for (const auto& entry : bucket_entries) {
            device_count[entry.device_name]++;
            severity_count[entry.severity]++;
        }

        // Find most frequent device
        auto max_device = std::max_element(device_count.begin(), device_count.end(),
            [](const auto& a, const auto& b) { return a.second < b.second; });
        if (max_device != device_count.end()) {
            event.device = max_device->first;
        }

        // Find highest severity
        std::vector<std::string> severity_order = {"emergency", "alert", "critical", "error", "warning", "notice", "info", "debug"};
        for (const auto& sev : severity_order) {
            if (severity_count.count(sev) > 0) {
                event.severity = sev;
                break;
            }
        }

        // Determine event type based on content analysis
        bool has_interface_event = false;
        bool has_routing_event = false;
        bool has_auth_event = false;
        bool has_hardware_event = false;

        for (const auto& entry : bucket_entries) {
            if (entry.message.find("interface") != std::string::npos ||
                entry.message.find("LINEPROTO") != std::string::npos) {
                has_interface_event = true;
            }
            if (entry.message.find("BGP") != std::string::npos ||
                entry.message.find("OSPF") != std::string::npos) {
                has_routing_event = true;
            }
            if (entry.message.find("login") != std::string::npos ||
                entry.message.find("auth") != std::string::npos) {
                has_auth_event = true;
            }
            if (entry.message.find("temperature") != std::string::npos ||
                entry.message.find("power") != std::string::npos ||
                entry.message.find("fan") != std::string::npos) {
                has_hardware_event = true;
            }
        }

        if (has_interface_event) {
            event.event_type = "interface";
        } else if (has_routing_event) {
            event.event_type = "routing";
        } else if (has_auth_event) {
            event.event_type = "security";
        } else if (has_hardware_event) {
            event.event_type = "hardware";
        } else {
            event.event_type = "general";
        }

        // Create summary
        std::ostringstream summary;
        summary << bucket_entries.size() << " events";
        if (device_count.size() > 1) {
            summary << " across " << device_count.size() << " devices";
        }
        event.summary = summary.str();

        events.push_back(event);
    }

    // Sort by timestamp
    std::sort(events.begin(), events.end(),
        [](const TimelineEvent& a, const TimelineEvent& b) {
            return a.timestamp < b.timestamp;
        });

    return events;
}

std::string TimelineVisualizer::generate_ascii_timeline(
    const std::vector<TimelineEvent>& events, int width) {

    if (events.empty()) {
        return "No events to display.\n";
    }

    std::ostringstream timeline;

    // Header
    timeline << "Network Events Timeline\n";
    timeline << std::string(width, '=') << "\n\n";

    // Time range info
    auto start_time = events.front().timestamp;
    auto end_time = events.back().timestamp;
    timeline << "Time Range: " << format_timestamp(start_time)
             << " to " << format_timestamp(end_time) << "\n";
    timeline << "Total Events: " << events.size() << "\n\n";

    // Timeline visualization
    for (const auto& event : events) {
        std::string timestamp_str = format_timestamp(event.timestamp);
        std::string severity_symbol = get_severity_symbol(event.severity);

        timeline << timestamp_str << " " << severity_symbol << " ";
        timeline << "[" << event.device << "] ";
        timeline << event.event_type << ": " << event.summary << "\n";

        // Show related entries if they're critical
        if (event.severity == "critical" || event.severity == "error" || event.severity == "emergency") {
            for (const auto& entry : event.related_entries) {
                if (entry.severity == "critical" || entry.severity == "error" || entry.severity == "emergency") {
                    timeline << "    └─ " << entry.message.substr(0, width - 10) << "\n";
                }
            }
        }
    }

    // Statistics
    timeline << "\n" << std::string(width, '-') << "\n";
    timeline << "Event Type Summary:\n";

    std::unordered_map<std::string, int> type_count;
    std::unordered_map<std::string, int> severity_count;

    for (const auto& event : events) {
        type_count[event.event_type]++;
        severity_count[event.severity]++;
    }

    for (const auto& [type, count] : type_count) {
        timeline << "  " << type << ": " << count << " events\n";
    }

    timeline << "\nSeverity Distribution:\n";
    for (const auto& [severity, count] : severity_count) {
        timeline << "  " << severity << ": " << count << " events "
                 << get_severity_symbol(severity) << "\n";
    }

    return timeline.str();
}

std::string TimelineVisualizer::generate_interactive_timeline(const std::vector<TimelineEvent>& events) {
    std::ostringstream interactive;

    interactive << "Interactive Network Timeline (ASCII)\n";
    interactive << std::string(80, '=') << "\n\n";

    if (events.empty()) {
        interactive << "No events available for timeline.\n";
        return interactive.str();
    }

    // Create visual timeline with bars
    auto start_time = events.front().timestamp;
    auto end_time = events.back().timestamp;
    auto total_duration = std::chrono::duration_cast<std::chrono::minutes>(end_time - start_time);

    interactive << "Timeline Span: " << total_duration.count() << " minutes\n";
    interactive << "Visual Scale: Each '|' represents ~" << (total_duration.count() / 60) << " minute(s)\n\n";

    // Create hourly buckets
    int timeline_width = 60;
    std::vector<int> activity_bars(timeline_width, 0);
    std::vector<std::string> severity_bars(timeline_width, ".");

    for (const auto& event : events) {
        auto event_offset = std::chrono::duration_cast<std::chrono::minutes>(event.timestamp - start_time);
        int bar_position = static_cast<int>((event_offset.count() * timeline_width) / total_duration.count());
        bar_position = std::max(0, std::min(bar_position, timeline_width - 1));

        activity_bars[bar_position] += static_cast<int>(event.related_entries.size());

        // Update severity indicator
        if (event.severity == "critical" || event.severity == "emergency") {
            severity_bars[bar_position] = "!";
        } else if (event.severity == "error" && severity_bars[bar_position] == ".") {
            severity_bars[bar_position] = "X";
        } else if (event.severity == "warning" &&
                   (severity_bars[bar_position] == "." || severity_bars[bar_position] == "-")) {
            severity_bars[bar_position] = "W";
        } else if (severity_bars[bar_position] == ".") {
            severity_bars[bar_position] = "-";
        }
    }

    // Draw activity graph
    interactive << "Activity Level:\n";
    int max_activity = *std::max_element(activity_bars.begin(), activity_bars.end());
    if (max_activity == 0) max_activity = 1;

    for (int level = 5; level > 0; --level) {
        interactive << std::setw(2) << level << " ";
        for (int bar : activity_bars) {
            if (bar >= (max_activity * level) / 5) {
                interactive << "█";
            } else {
                interactive << " ";
            }
        }
        interactive << "\n";
    }

    // Timeline base
    interactive << "   ";
    for (int i = 0; i < timeline_width; ++i) {
        interactive << "─";
    }
    interactive << "\n";

    // Severity indicators
    interactive << "   ";
    for (const auto& sev : severity_bars) {
        interactive << sev;
    }
    interactive << "\n\n";

    // Legend
    interactive << "Legend:\n";
    interactive << "  !  = Critical/Emergency   X = Error   W = Warning   - = Info/Debug\n";
    interactive << "  █  = High Activity        ▓ = Medium  ▒ = Low       · = Minimal\n\n";

    // Recent critical events
    interactive << "Recent Critical Events:\n";
    interactive << std::string(40, '-') << "\n";

    int critical_count = 0;
    for (auto it = events.rbegin(); it != events.rend() && critical_count < 5; ++it) {
        if (it->severity == "critical" || it->severity == "error" || it->severity == "emergency") {
            interactive << format_timestamp(it->timestamp) << " ";
            interactive << get_severity_symbol(it->severity) << " ";
            interactive << "[" << it->device << "] " << it->summary << "\n";
            critical_count++;
        }
    }

    if (critical_count == 0) {
        interactive << "No critical events in recent timeline.\n";
    }

    return interactive.str();
}

std::string TimelineVisualizer::generate_correlation_graph(const std::vector<LogEntry>& entries) {
    std::ostringstream graph;

    graph << "Event Correlation Analysis\n";
    graph << std::string(50, '=') << "\n\n";

    // Analyze correlations by time proximity
    std::unordered_map<std::string, std::unordered_map<std::string, int>> correlations;
    auto time_window = std::chrono::minutes(5);

    for (size_t i = 0; i < entries.size(); ++i) {
        for (size_t j = i + 1; j < entries.size(); ++j) {
            if (entries[j].timestamp - entries[i].timestamp > time_window) {
                break;
            }

            std::string event1 = entries[i].device_name + ":" + entries[i].severity;
            std::string event2 = entries[j].device_name + ":" + entries[j].severity;

            correlations[event1][event2]++;
            correlations[event2][event1]++;
        }
    }

    // Display significant correlations
    graph << "Significant Event Correlations (within 5 minutes):\n\n";
    for (const auto& [event1, related] : correlations) {
        for (const auto& [event2, count] : related) {
            if (count >= 3 && event1 < event2) { // Avoid duplicates and show significant correlations
                graph << event1 << " <──(" << count << " times)──> " << event2 << "\n";
            }
        }
    }

    return graph.str();
}

std::string TimelineVisualizer::generate_device_interaction_map(const std::vector<LogEntry>& entries) {
    std::ostringstream map;

    map << "Device Interaction Map\n";
    map << std::string(40, '=') << "\n\n";

    // Count events per device
    std::unordered_map<std::string, int> device_activity;
    std::unordered_map<std::string, std::unordered_map<std::string, int>> device_severity;

    for (const auto& entry : entries) {
        device_activity[entry.device_name]++;
        device_severity[entry.device_name][entry.severity]++;
    }

    // Sort devices by activity
    std::vector<std::pair<std::string, int>> sorted_devices(device_activity.begin(), device_activity.end());
    std::sort(sorted_devices.begin(), sorted_devices.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });

    map << "Device Activity Summary:\n\n";
    for (const auto& [device, activity] : sorted_devices) {
        map << std::setw(15) << device << " ";

        // Activity bar
        int bar_length = std::min(30, activity / 2);
        map << "[" << std::string(bar_length, '█') << std::string(30 - bar_length, ' ') << "] ";
        map << activity << " events\n";

        // Severity breakdown
        auto& severities = device_severity[device];
        map << std::setw(15) << "" << " Errors:" << severities["error"]
            << " Warnings:" << severities["warning"]
            << " Info:" << severities["info"] << "\n\n";
    }

    return map.str();
}

std::string TimelineVisualizer::format_timestamp(const std::chrono::system_clock::time_point& tp) {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%m-%d %H:%M");
    return oss.str();
}

std::string TimelineVisualizer::get_severity_symbol(const std::string& severity) {
    if (severity == "emergency" || severity == "alert") return "🚨";
    if (severity == "critical") return "❌";
    if (severity == "error") return "🔴";
    if (severity == "warning") return "⚠️";
    if (severity == "notice") return "📋";
    if (severity == "info") return "ℹ️";
    if (severity == "debug") return "🔧";
    return "•";
}

} // namespace netlogai::analytics