#include "analytics/git_style_commands.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <regex>
#include <memory>

namespace netlogai::analytics {

// Static instances
static std::shared_ptr<LogRepository> g_repository = nullptr;
static std::shared_ptr<PatternAnalyzer> g_analyzer = nullptr;
static std::shared_ptr<TimelineVisualizer> g_visualizer = nullptr;

// LogRepository Implementation
LogRepository::LogRepository(const std::string& base_path) : base_path_(base_path) {
    load_entries();
}

void LogRepository::load_entries() {
    entries_.clear();

    // Load from multiple sources
    std::vector<std::string> log_sources = {
        base_path_ + "/device_logs",
        base_path_ + "/archived_logs",
        "logs", // Current directory
        ".netlogai/logs" // User config directory
    };

    for (const auto& source_dir : log_sources) {
        if (!std::filesystem::exists(source_dir)) continue;

        for (const auto& entry : std::filesystem::recursive_directory_iterator(source_dir)) {
            if (entry.is_regular_file() &&
                (entry.path().extension() == ".log" || entry.path().extension() == ".txt")) {

                std::ifstream file(entry.path());
                std::string line;
                std::string device_name = entry.path().stem().string();

                while (std::getline(file, line)) {
                    if (line.empty()) continue;

                    LogEntry log_entry = parse_log_line_to_entry(line, device_name);
                    if (!log_entry.id.empty()) {
                        entries_.push_back(log_entry);
                    }
                }
            }
        }
    }

    // Sort entries by timestamp
    std::sort(entries_.begin(), entries_.end(),
        [](const LogEntry& a, const LogEntry& b) {
            return a.timestamp < b.timestamp;
        });

    // Generate sequential IDs for git-style referencing
    for (size_t i = 0; i < entries_.size(); ++i) {
        entries_[i].id = "log-" + std::to_string(i);
        entries_[i].hash = generate_hash(entries_[i]);
    }
}

LogEntry LogRepository::parse_log_line_to_entry(const std::string& line, const std::string& device) {
    LogEntry entry;

    // Basic syslog parsing
    std::regex timestamp_regex(R"((\w+\s+\d+\s+\d+:\d+:\d+))");
    std::regex severity_regex(R"(%([A-Z]+)-(\d)-([A-Z]+):)");
    std::regex interface_regex(R"(Interface\s+([A-Za-z0-9\/]+))");

    entry.device_name = device;
    entry.raw_line = line;
    entry.timestamp = std::chrono::system_clock::now(); // Default to now

    // Extract severity
    std::smatch severity_match;
    if (std::regex_search(line, severity_match, severity_regex)) {
        int level = std::stoi(severity_match[2].str());
        switch (level) {
            case 0: case 1: entry.severity = "critical"; break;
            case 2: entry.severity = "error"; break;
            case 3: entry.severity = "error"; break;
            case 4: entry.severity = "warning"; break;
            case 5: entry.severity = "notice"; break;
            case 6: entry.severity = "info"; break;
            case 7: entry.severity = "debug"; break;
            default: entry.severity = "info"; break;
        }
    } else {
        // Fallback severity detection
        std::string lower_line = line;
        std::transform(lower_line.begin(), lower_line.end(), lower_line.begin(), ::tolower);

        if (lower_line.find("critical") != std::string::npos ||
            lower_line.find("emergency") != std::string::npos) {
            entry.severity = "critical";
        } else if (lower_line.find("error") != std::string::npos ||
                   lower_line.find("fail") != std::string::npos) {
            entry.severity = "error";
        } else if (lower_line.find("warn") != std::string::npos) {
            entry.severity = "warning";
        } else {
            entry.severity = "info";
        }
    }

    // Extract interface
    std::smatch interface_match;
    if (std::regex_search(line, interface_match, interface_regex)) {
        entry.interface = interface_match[1].str();
    }

    // Extract message (everything after the facility-level indicator or the full line)
    auto colon_pos = line.find(": ");
    if (colon_pos != std::string::npos) {
        entry.message = line.substr(colon_pos + 2);
    } else {
        entry.message = line;
    }

    return entry;
}

std::string LogRepository::generate_hash(const LogEntry& entry) {
    // Simple hash generation (in production, use proper SHA-256)
    std::hash<std::string> hasher;
    std::string to_hash = entry.device_name + entry.message + entry.severity;
    return "hash-" + std::to_string(hasher(to_hash) % 1000000);
}

std::vector<LogEntry> LogRepository::log(int count, const std::string& device_filter) {
    std::vector<LogEntry> result;

    for (auto it = entries_.rbegin(); it != entries_.rend() && result.size() < static_cast<size_t>(count); ++it) {
        if (device_filter.empty() || it->device_name == device_filter) {
            result.push_back(*it);
        }
    }

    return result;
}

std::vector<LogEntry> LogRepository::show(const std::string& ref, int offset) {
    std::vector<LogEntry> result;

    if (entries_.empty()) return result;

    int start_index = static_cast<int>(entries_.size()) - 1 - offset;
    if (ref == "HEAD") {
        // Show from HEAD minus offset
        if (start_index >= 0 && start_index < static_cast<int>(entries_.size())) {
            for (int i = start_index; i >= std::max(0, start_index - 10); --i) {
                result.push_back(entries_[i]);
            }
        }
    }

    return result;
}

std::vector<LogEntry> LogRepository::diff(const std::string& from_ref, const std::string& to_ref) {
    // For now, implement as device comparison
    std::string device1 = from_ref;
    std::string device2 = to_ref;

    std::vector<LogEntry> device1_entries, device2_entries;

    for (const auto& entry : entries_) {
        if (entry.device_name == device1) {
            device1_entries.push_back(entry);
        } else if (entry.device_name == device2) {
            device2_entries.push_back(entry);
        }
    }

    // Return combined entries for comparison
    std::vector<LogEntry> result = device1_entries;
    result.insert(result.end(), device2_entries.begin(), device2_entries.end());

    // Sort by timestamp for comparison
    std::sort(result.begin(), result.end(),
        [](const LogEntry& a, const LogEntry& b) {
            return a.timestamp < b.timestamp;
        });

    return result;
}

std::vector<LogEntry> LogRepository::blame(const std::string& interface, const std::string& device) {
    std::vector<LogEntry> result;

    for (const auto& entry : entries_) {
        if (entry.interface == interface &&
            (device.empty() || entry.device_name == device)) {
            result.push_back(entry);
        }
    }

    // Sort by timestamp (most recent first)
    std::sort(result.begin(), result.end(),
        [](const LogEntry& a, const LogEntry& b) {
            return a.timestamp > b.timestamp;
        });

    return result;
}

std::string LogRepository::generate_graph(const std::string& device_filter, int hours) {
    auto now = std::chrono::system_clock::now();
    auto start_time = now - std::chrono::hours(hours);

    std::vector<LogEntry> filtered_entries;
    for (const auto& entry : entries_) {
        if (entry.timestamp >= start_time &&
            (device_filter.empty() || entry.device_name == device_filter)) {
            filtered_entries.push_back(entry);
        }
    }

    TimelineVisualizer visualizer;
    auto events = visualizer.create_timeline(filtered_entries);
    return visualizer.generate_ascii_timeline(events, 80);
}

// GitStyleCommands Implementation
void GitStyleCommands::register_commands(cli::CommandLine& cli) {
    // Git-style log commands
    cli.register_command("log", cmd_log, "Git-style log viewing with advanced filtering");
    cli.register_command("show", cmd_show, "Show specific log entries (e.g., show HEAD~5)");
    cli.register_command("diff", cmd_diff, "Compare logs between devices or time periods");
    cli.register_command("blame", cmd_blame, "Show interface-specific issue history");

    // Advanced analytics commands
    cli.register_command("analyze", cmd_analyze, "Advanced pattern analysis and anomaly detection");
    cli.register_command("correlate", cmd_correlate, "Find correlated events across devices");
    cli.register_command("timeline", cmd_timeline, "Interactive timeline visualization");
}

int GitStyleCommands::cmd_log(const cli::CommandArgs& args) {
    bool online = args.has_flag("online");
    bool graph = args.has_flag("graph");
    std::string device = args.get_option("device");
    std::string grep_pattern = args.get_option("grep");
    int count = std::stoi(args.get_option("n", "10"));

    auto repo = get_repository();

    if (graph) {
        std::string graph_output = repo->generate_graph(device, 24);
        std::cout << graph_output << std::endl;
        return 0;
    }

    auto entries = repo->log(count, device);

    std::cout << "Network Log History";
    if (!device.empty()) {
        std::cout << " (Device: " << device << ")";
    }
    std::cout << "\n" << std::string(50, '=') << "\n\n";

    for (const auto& entry : entries) {
        if (!grep_pattern.empty()) {
            if (entry.message.find(grep_pattern) == std::string::npos) {
                continue;
            }
        }

        if (online) {
            // Condensed format
            auto time_t = std::chrono::system_clock::to_time_t(entry.timestamp);
            std::cout << std::put_time(std::localtime(&time_t), "%m-%d %H:%M") << " ";
            std::cout << "[" << entry.device_name << "] ";
            std::cout << entry.severity.substr(0, 1) << " " << entry.message.substr(0, 60) << "\n";
        } else {
            // Full format
            auto time_t = std::chrono::system_clock::to_time_t(entry.timestamp);
            std::cout << "commit " << entry.hash << "\n";
            std::cout << "Date:   " << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << "\n";
            std::cout << "Device: " << entry.device_name << "\n";
            std::cout << "Severity: " << entry.severity << "\n";
            if (!entry.interface.empty()) {
                std::cout << "Interface: " << entry.interface << "\n";
            }
            std::cout << "\n    " << entry.message << "\n\n";
        }
    }

    return 0;
}

int GitStyleCommands::cmd_show(const cli::CommandArgs& args) {
    std::string ref = args.get_arg(0, "HEAD");
    int offset = 0;

    // Parse HEAD~N syntax
    if (ref.find("HEAD~") == 0) {
        offset = std::stoi(ref.substr(5));
        ref = "HEAD";
    }

    auto repo = get_repository();
    auto entries = repo->show(ref, offset);

    std::cout << "Showing log entries at " << ref;
    if (offset > 0) {
        std::cout << "~" << offset;
    }
    std::cout << "\n" << std::string(40, '=') << "\n\n";

    for (const auto& entry : entries) {
        auto time_t = std::chrono::system_clock::to_time_t(entry.timestamp);
        std::cout << "commit " << entry.hash << "\n";
        std::cout << "Date:   " << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << "\n";
        std::cout << "Device: " << entry.device_name << "\n";
        std::cout << "Severity: " << entry.severity << "\n";
        std::cout << "\n    " << entry.message << "\n\n";
    }

    return 0;
}

int GitStyleCommands::cmd_diff(const cli::CommandArgs& args) {
    if (args.args.size() < 1) {
        std::cerr << "Usage: netlogai diff <device1>..<device2>\n";
        return 1;
    }

    std::string range = args.args[0];
    auto dot_pos = range.find("..");
    if (dot_pos == std::string::npos) {
        std::cerr << "Error: Invalid range format. Use device1..device2\n";
        return 1;
    }

    std::string device1 = range.substr(0, dot_pos);
    std::string device2 = range.substr(dot_pos + 2);

    auto repo = get_repository();
    auto entries = repo->diff(device1, device2);

    std::cout << "Comparing logs: " << device1 << " vs " << device2 << "\n";
    std::cout << std::string(50, '=') << "\n\n";

    for (const auto& entry : entries) {
        auto time_t = std::chrono::system_clock::to_time_t(entry.timestamp);
        std::string prefix = (entry.device_name == device1) ? "- " : "+ ";

        std::cout << prefix << std::put_time(std::localtime(&time_t), "%H:%M:%S") << " ";
        std::cout << "[" << entry.device_name << "] " << entry.message << "\n";
    }

    return 0;
}

int GitStyleCommands::cmd_blame(const cli::CommandArgs& args) {
    if (args.args.empty()) {
        std::cerr << "Usage: netlogai blame interface <interface-name> [device]\n";
        return 1;
    }

    std::string interface_name = args.get_arg(1);
    std::string device = args.get_arg(2, "");

    if (interface_name.empty()) {
        std::cerr << "Error: Interface name required\n";
        return 1;
    }

    auto repo = get_repository();
    auto entries = repo->blame(interface_name, device);

    std::cout << "Interface blame analysis: " << interface_name;
    if (!device.empty()) {
        std::cout << " on " << device;
    }
    std::cout << "\n" << std::string(50, '=') << "\n\n";

    if (entries.empty()) {
        std::cout << "No entries found for interface " << interface_name << "\n";
        return 0;
    }

    for (const auto& entry : entries) {
        auto time_t = std::chrono::system_clock::to_time_t(entry.timestamp);
        std::cout << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << " ";
        std::cout << "(" << entry.device_name << ") " << entry.severity << ": ";
        std::cout << entry.message << "\n";
    }

    return 0;
}

int GitStyleCommands::cmd_analyze(const cli::CommandArgs& args) {
    std::string pattern = args.get_option("pattern");
    bool anomalies = args.has_flag("anomalies");

    auto repo = get_repository();
    auto analyzer = get_analyzer();

    auto entries = repo->log(1000); // Analyze recent entries

    std::cout << "Advanced Log Analysis\n";
    std::cout << std::string(30, '=') << "\n\n";

    if (!pattern.empty()) {
        auto matches = analyzer->find_pattern(pattern, entries);
        std::cout << "Pattern Analysis for: " << pattern << "\n";
        std::cout << "Found " << matches.size() << " matches:\n\n";

        for (const auto& match : matches) {
            auto time_t = std::chrono::system_clock::to_time_t(match.entry.timestamp);
            std::cout << std::put_time(std::localtime(&time_t), "%H:%M:%S") << " ";
            std::cout << "[" << match.entry.device_name << "] ";
            std::cout << match.entry.message << "\n";
        }
    } else {
        auto patterns = analyzer->analyze_patterns(entries);
        std::cout << "Pattern Detection Results:\n";
        std::cout << "Found " << patterns.size() << " pattern matches:\n\n";

        for (size_t i = 0; i < std::min(patterns.size(), size_t(10)); ++i) {
            const auto& match = patterns[i];
            std::cout << "• " << match.pattern.name << " (confidence: "
                     << std::fixed << std::setprecision(1) << match.confidence * 100 << "%)\n";
            std::cout << "  " << match.pattern.description << "\n";
            auto time_t = std::chrono::system_clock::to_time_t(match.entry.timestamp);
            std::cout << "  " << std::put_time(std::localtime(&time_t), "%H:%M:%S")
                     << " [" << match.entry.device_name << "]\n\n";
        }
    }

    if (anomalies) {
        auto anomaly_entries = analyzer->detect_anomalies(entries);
        std::cout << "\nAnomaly Detection:\n";
        std::cout << "Found " << anomaly_entries.size() << " anomalies:\n\n";

        for (const auto& entry : anomaly_entries) {
            auto time_t = std::chrono::system_clock::to_time_t(entry.timestamp);
            std::cout << "⚠️  " << std::put_time(std::localtime(&time_t), "%H:%M:%S") << " ";
            std::cout << "[" << entry.device_name << "] " << entry.message << "\n";
        }
    }

    return 0;
}

int GitStyleCommands::cmd_correlate(const cli::CommandArgs& args) {
    int timespan = std::stoi(args.get_option("timespan", "60")); // minutes

    auto repo = get_repository();
    auto analyzer = get_analyzer();

    auto entries = repo->log(500);
    auto correlated = analyzer->find_correlations(entries, timespan);

    std::cout << "Event Correlation Analysis\n";
    std::cout << std::string(35, '=') << "\n\n";
    std::cout << "Time window: " << timespan << " minutes\n";
    std::cout << "Found " << correlated.size() << " correlated events:\n\n";

    for (const auto& entry : correlated) {
        auto time_t = std::chrono::system_clock::to_time_t(entry.timestamp);
        std::cout << std::put_time(std::localtime(&time_t), "%H:%M:%S") << " ";
        std::cout << "[" << entry.device_name << "] " << entry.severity << ": ";
        std::cout << entry.message << "\n";
    }

    return 0;
}

int GitStyleCommands::cmd_timeline(const cli::CommandArgs& args) {
    bool interactive = args.has_flag("interactive");
    int hours = std::stoi(args.get_option("hours", "24"));

    auto repo = get_repository();
    auto visualizer = get_visualizer();

    auto entries = repo->log(1000);
    auto events = visualizer->create_timeline(entries);

    if (interactive) {
        std::cout << visualizer->generate_interactive_timeline(events);
    } else {
        std::cout << visualizer->generate_ascii_timeline(events, 80);
    }

    return 0;
}

// Helper functions
std::shared_ptr<LogRepository> GitStyleCommands::get_repository() {
    if (!g_repository) {
        g_repository = std::make_shared<LogRepository>(".");
    }
    return g_repository;
}

std::shared_ptr<PatternAnalyzer> GitStyleCommands::get_analyzer() {
    if (!g_analyzer) {
        g_analyzer = std::make_shared<PatternAnalyzer>();
    }
    return g_analyzer;
}

std::shared_ptr<TimelineVisualizer> GitStyleCommands::get_visualizer() {
    if (!g_visualizer) {
        g_visualizer = std::make_shared<TimelineVisualizer>();
    }
    return g_visualizer;
}

} // namespace netlogai::analytics