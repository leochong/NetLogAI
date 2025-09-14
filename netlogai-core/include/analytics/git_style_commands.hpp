#pragma once

#include "../../src/cli/command_line.hpp"
#include <string>
#include <vector>
#include <chrono>
#include <memory>

namespace netlogai::analytics {

// Log entry with git-style metadata
struct LogEntry {
    std::string id;                    // Unique hash identifier (like git commit)
    std::chrono::system_clock::time_point timestamp;
    std::string device_name;
    std::string interface;             // For interface-specific tracking
    std::string severity;
    std::string message;
    std::string raw_line;
    std::vector<std::string> tags;     // For categorization
    std::string hash;                  // SHA-like hash for versioning
};

// Git-style log repository for versioned log management
class LogRepository {
public:
    LogRepository(const std::string& base_path);

    // Git-style operations
    std::vector<LogEntry> log(int count = 10, const std::string& device_filter = "");
    std::vector<LogEntry> show(const std::string& ref = "HEAD", int offset = 0);
    std::vector<LogEntry> diff(const std::string& from_ref, const std::string& to_ref);
    std::vector<LogEntry> blame(const std::string& interface, const std::string& device = "");

    // Timeline and graph operations
    std::string generate_graph(const std::string& device_filter = "", int hours = 24);
    std::vector<LogEntry> get_timeline(const std::chrono::system_clock::time_point& start,
                                     const std::chrono::system_clock::time_point& end);

    // Branch-like operations for device groups
    std::vector<std::string> list_device_groups();
    std::vector<LogEntry> get_device_group_logs(const std::string& group_name);

private:
    std::string base_path_;
    std::vector<LogEntry> entries_;

    void load_entries();
    std::string generate_hash(const LogEntry& entry);
    LogEntry parse_log_line_to_entry(const std::string& line, const std::string& device);
};

// Advanced pattern analysis engine
class PatternAnalyzer {
public:
    struct Pattern {
        std::string name;
        std::string regex;
        std::string description;
        int severity_weight;
        std::vector<std::string> related_patterns;
    };

    struct PatternMatch {
        Pattern pattern;
        LogEntry entry;
        double confidence;
        std::vector<std::string> extracted_values;
    };

    PatternAnalyzer();

    // Pattern detection
    std::vector<PatternMatch> analyze_patterns(const std::vector<LogEntry>& entries);
    std::vector<PatternMatch> find_pattern(const std::string& pattern_name,
                                         const std::vector<LogEntry>& entries);

    // Advanced analysis
    std::vector<LogEntry> find_correlations(const std::vector<LogEntry>& entries,
                                          int time_window_minutes = 5);
    std::vector<LogEntry> detect_anomalies(const std::vector<LogEntry>& entries);
    std::map<std::string, int> generate_statistics(const std::vector<LogEntry>& entries);

private:
    std::vector<Pattern> patterns_;
    void load_default_patterns();
};

// Timeline visualization system
class TimelineVisualizer {
public:
    struct TimelineEvent {
        std::chrono::system_clock::time_point timestamp;
        std::string device;
        std::string event_type;
        std::string severity;
        std::string summary;
        std::vector<LogEntry> related_entries;
    };

    TimelineVisualizer();

    // Visualization methods
    std::string generate_ascii_timeline(const std::vector<TimelineEvent>& events,
                                       int width = 80);
    std::string generate_interactive_timeline(const std::vector<TimelineEvent>& events);
    std::vector<TimelineEvent> create_timeline(const std::vector<LogEntry>& entries);

    // Correlation visualization
    std::string generate_correlation_graph(const std::vector<LogEntry>& entries);
    std::string generate_device_interaction_map(const std::vector<LogEntry>& entries);

private:
    std::string format_timestamp(const std::chrono::system_clock::time_point& tp);
    std::string get_severity_symbol(const std::string& severity);
};

// Git-style command implementations
class GitStyleCommands {
public:
    static void register_commands(cli::CommandLine& cli);

private:
    // Git-style log commands
    static int cmd_log(const cli::CommandArgs& args);           // netlogai log [options]
    static int cmd_show(const cli::CommandArgs& args);         // netlogai show HEAD~5
    static int cmd_diff(const cli::CommandArgs& args);         // netlogai diff device1..device2
    static int cmd_blame(const cli::CommandArgs& args);        // netlogai blame interface gi0/1

    // Advanced analytics
    static int cmd_analyze(const cli::CommandArgs& args);      // netlogai analyze --pattern "BGP"
    static int cmd_correlate(const cli::CommandArgs& args);    // netlogai correlate --timespan 1h
    static int cmd_timeline(const cli::CommandArgs& args);     // netlogai timeline --interactive

    // Helper functions
    static std::shared_ptr<LogRepository> get_repository();
    static std::shared_ptr<PatternAnalyzer> get_analyzer();
    static std::shared_ptr<TimelineVisualizer> get_visualizer();
    static void show_git_style_help();
};

} // namespace netlogai::analytics