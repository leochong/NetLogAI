#include "log_commands.hpp"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <chrono>
#include <thread>

#ifdef LIBNETLOG_ENABLE_LUA
#include "libnetlog/lua_engine.hpp"
#include "libnetlog/parsers/lua_parser.hpp"
#include "libnetlog/parser_factory.hpp"
#include "libnetlog/log_entry.hpp"
#endif

namespace netlogai::commands {

void LogCommands::register_commands(cli::CommandLine& cli) {
    cli.register_command("log", [](const cli::CommandArgs&) {
        show_log_help();
        return 0;
    }, "View and manage network logs");

    cli.register_subcommand("log", "parse", parse_logs, "Parse log files using available parsers");
    cli.register_subcommand("log", "analyze", analyze_logs, "Analyze parsed logs for patterns");
    cli.register_subcommand("log", "show", show_logs, "Display log entries with formatting");
    cli.register_subcommand("log", "filter", filter_logs, "Filter logs by criteria");
    cli.register_subcommand("log", "export", export_logs, "Export logs to various formats");
    cli.register_subcommand("log", "tail", tail_logs, "Follow log files in real-time");
}

int LogCommands::parse_logs(const cli::CommandArgs& args) {
    std::string input_file = args.get_option("input", args.get_arg(0));
    std::string output_file = args.get_option("output");
    std::string parser_name = args.get_option("parser");
    bool verbose = args.has_flag("verbose") || args.has_flag("v");

    if (input_file.empty()) {
        std::cerr << "Error: No input file specified.\n";
        std::cerr << "Usage: netlogai log parse <input-file> [--parser <name>] [--output <file>]\n";
        return 1;
    }

    if (!is_valid_log_file(input_file)) {
        std::cerr << "Error: Input file not found or not readable: " << input_file << "\n";
        return 1;
    }

    std::cout << "Parsing log file: " << input_file << "\n";
    if (!parser_name.empty()) {
        std::cout << "Using parser: " << parser_name << "\n";
    } else {
        std::cout << "Auto-detecting parser...\n";
    }

#ifdef LIBNETLOG_ENABLE_LUA
    try {
        std::ifstream file(input_file);
        if (!file) {
            std::cerr << "Error: Cannot open input file: " << input_file << "\n";
            return 1;
        }

        std::ofstream output_stream;
        if (!output_file.empty()) {
            output_stream.open(output_file);
            if (!output_stream) {
                std::cerr << "Error: Cannot create output file: " << output_file << "\n";
                return 1;
            }
        }

        std::ostream& out = output_file.empty() ? std::cout : output_stream;

        std::string line;
        int line_number = 0;
        int parsed_count = 0;
        int total_count = 0;

        // Create parser registry and load parsers
        libnetlog::LuaParserRegistry registry;

        // Try to load parsers from standard locations
        std::vector<std::string> parser_dirs = {
            "examples/parsers/cisco",
            "examples/parsers/generic",
            ".netlogai/parsers"
        };

        for (const auto& dir : parser_dirs) {
            if (std::filesystem::exists(dir)) {
                registry.load_parsers_from_directory(dir);
            }
        }

        if (verbose) {
            auto parser_names = registry.list_parsers();
            std::cout << "Loaded " << parser_names.size() << " parsers: ";
            for (size_t i = 0; i < parser_names.size(); ++i) {
                std::cout << parser_names[i];
                if (i < parser_names.size() - 1) std::cout << ", ";
            }
            std::cout << "\n\n";
        }

        out << "{\n  \"parsed_entries\": [\n";
        bool first_entry = true;

        while (std::getline(file, line)) {
            line_number++;
            total_count++;

            if (line.empty()) continue;

            // Find appropriate parser
            auto* parser = registry.find_parser_for_message(line);
            if (parser) {
                auto entry = parser->parse(line);
                if (entry.has_value()) {
                    parsed_count++;

                    if (!first_entry) {
                        out << ",\n";
                    }
                    first_entry = false;

                    // Output as JSON
                    out << "    {\n";
                    out << "      \"line_number\": " << line_number << ",\n";
                    out << "      \"parser\": \"" << parser->get_parser_name() << "\",\n";
                    out << "      \"timestamp\": \"" << std::chrono::duration_cast<std::chrono::seconds>(entry->timestamp().time_since_epoch()).count() << "\",\n";
                    out << "      \"severity\": \"" << static_cast<int>(entry->severity()) << "\",\n";
                    out << "      \"facility\": \"" << entry->facility() << "\",\n";
                    out << "      \"message\": \"" << entry->message() << "\",\n";
                    out << "      \"hostname\": \"" << entry->hostname() << "\",\n";
                    out << "      \"raw_message\": \"" << line << "\"\n";
                    out << "    }";

                    if (verbose && output_file.empty()) {
                        std::cout << "Line " << line_number << ": PARSED by " << parser->get_parser_name() << "\n";
                    }
                }
            } else if (verbose && output_file.empty()) {
                std::cout << "Line " << line_number << ": NO_PARSER\n";
            }
        }

        out << "\n  ],\n";
        out << "  \"summary\": {\n";
        out << "    \"total_lines\": " << total_count << ",\n";
        out << "    \"parsed_lines\": " << parsed_count << ",\n";
        out << "    \"parse_rate\": " << std::fixed << std::setprecision(1)
            << (total_count > 0 ? (100.0 * parsed_count / total_count) : 0.0) << "\n";
        out << "  }\n";
        out << "}\n";

        std::cout << "\nParsing Summary:\n";
        std::cout << "================\n";
        std::cout << "Total lines: " << total_count << "\n";
        std::cout << "Successfully parsed: " << parsed_count << "\n";
        std::cout << "Parse rate: " << std::fixed << std::setprecision(1)
                  << (total_count > 0 ? (100.0 * parsed_count / total_count) : 0.0) << "%\n";

        if (!output_file.empty()) {
            std::cout << "Results saved to: " << output_file << "\n";
        }

        return (parsed_count > 0) ? 0 : 1;

    } catch (const std::exception& e) {
        std::cerr << "Error: Parsing failed: " << e.what() << "\n";
        return 1;
    }
#else
    std::cerr << "Error: Lua scripting not available in this build\n";
    return 1;
#endif
}

int LogCommands::analyze_logs(const cli::CommandArgs& args) {
    std::string input_file = args.get_option("input", args.get_arg(0));
    std::string pattern = args.get_option("pattern");
    std::string timespan = args.get_option("timespan", "1h");
    bool correlate = args.has_flag("correlate");

    if (input_file.empty()) {
        std::cerr << "Error: No input file specified.\n";
        std::cerr << "Usage: netlogai log analyze <input-file> [--pattern <regex>] [--timespan <time>]\n";
        return 1;
    }

    std::cout << "Analyzing log file: " << input_file << "\n";
    if (!pattern.empty()) {
        std::cout << "Looking for pattern: " << pattern << "\n";
    }
    std::cout << "Time span: " << timespan << "\n";

    // Basic analysis implementation
    try {
        std::ifstream file(input_file);
        if (!file) {
            std::cerr << "Error: Cannot open input file: " << input_file << "\n";
            return 1;
        }

        std::string line;
        int line_count = 0;
        int error_count = 0;
        int warning_count = 0;

        std::cout << "\nAnalysis Results:\n";
        std::cout << "=================\n";

        while (std::getline(file, line)) {
            line_count++;

            // Simple pattern matching for demonstration
            if (line.find("error") != std::string::npos ||
                line.find("ERROR") != std::string::npos ||
                line.find("failed") != std::string::npos) {
                error_count++;
            } else if (line.find("warn") != std::string::npos ||
                      line.find("WARN") != std::string::npos) {
                warning_count++;
            }
        }

        std::cout << "Total lines analyzed: " << line_count << "\n";
        std::cout << "Error indicators: " << error_count << "\n";
        std::cout << "Warning indicators: " << warning_count << "\n";
        std::cout << "Error rate: " << std::fixed << std::setprecision(2)
                  << (line_count > 0 ? (100.0 * error_count / line_count) : 0.0) << "%\n";

        if (correlate) {
            std::cout << "\nCorrelation analysis would be performed here.\n";
            std::cout << "Advanced pattern detection and timeline correlation.\n";
        }

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: Analysis failed: " << e.what() << "\n";
        return 1;
    }
}

int LogCommands::show_logs(const cli::CommandArgs& args) {
    std::string input_file = args.get_option("input", args.get_arg(0));
    int lines = std::stoi(args.get_option("lines", "10"));
    bool follow = args.has_flag("follow") || args.has_flag("f");

    if (input_file.empty()) {
        input_file = get_default_log_directory() + "/latest.log";
    }

    if (!is_valid_log_file(input_file)) {
        std::cerr << "Error: Log file not found: " << input_file << "\n";
        return 1;
    }

    std::cout << "Showing logs from: " << input_file << "\n";
    std::cout << "Lines to display: " << (follow ? "all (following)" : std::to_string(lines)) << "\n\n";

    try {
        std::ifstream file(input_file);
        if (!file) {
            std::cerr << "Error: Cannot open log file: " << input_file << "\n";
            return 1;
        }

        if (follow) {
            // Simple tail -f implementation
            std::string line;
            auto last_size = std::filesystem::file_size(input_file);

            while (true) {
                while (std::getline(file, line)) {
                    print_log_entry(line, args.has_flag("verbose"));
                }

                if (file.eof()) {
                    file.clear();
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));

                    auto current_size = std::filesystem::file_size(input_file);
                    if (current_size < last_size) {
                        // File was truncated or rotated
                        file.close();
                        file.open(input_file);
                    }
                    last_size = current_size;
                }
            }
        } else {
            // Show last N lines
            std::vector<std::string> recent_lines;
            std::string line;

            while (std::getline(file, line)) {
                recent_lines.push_back(line);
                if (recent_lines.size() > static_cast<size_t>(lines)) {
                    recent_lines.erase(recent_lines.begin());
                }
            }

            for (const auto& log_line : recent_lines) {
                print_log_entry(log_line, args.has_flag("verbose"));
            }
        }

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: Failed to show logs: " << e.what() << "\n";
        return 1;
    }
}

int LogCommands::filter_logs(const cli::CommandArgs& args) {
    std::cout << "Log filtering functionality\n";
    std::cout << "===========================\n";
    std::cout << "Filter logs by:\n";
    std::cout << "• Severity level (--severity error,warning,info)\n";
    std::cout << "• Time range (--from 2024-01-01 --to 2024-01-31)\n";
    std::cout << "• Device type (--device cisco-ios,cisco-nxos)\n";
    std::cout << "• Pattern matching (--grep \"pattern\")\n\n";
    std::cout << "This feature will be implemented in the next phase.\n";
    return 0;
}

int LogCommands::export_logs(const cli::CommandArgs& args) {
    std::cout << "Log export functionality\n";
    std::cout << "========================\n";
    std::cout << "Export logs to formats:\n";
    std::cout << "• JSON (--format json)\n";
    std::cout << "• CSV (--format csv)\n";
    std::cout << "• XML (--format xml)\n";
    std::cout << "• SIEM formats (--format splunk,elasticsearch)\n\n";
    std::cout << "This feature will be implemented in the next phase.\n";
    return 0;
}

int LogCommands::tail_logs(const cli::CommandArgs& args) {
    // Redirect to show_logs with follow flag
    cli::CommandArgs modified_args = args;
    modified_args.flags.push_back("follow");
    return show_logs(modified_args);
}

void LogCommands::show_log_help() {
    std::cout << "NetLogAI Log Management\n";
    std::cout << "=======================\n\n";
    std::cout << "Usage: netlogai log <subcommand> [options]\n\n";
    std::cout << "Subcommands:\n";
    std::cout << "  parse <file>            Parse log file using available parsers\n";
    std::cout << "  analyze <file>          Analyze logs for patterns and anomalies\n";
    std::cout << "  show [file]             Display log entries with formatting\n";
    std::cout << "  filter <criteria>       Filter logs by various criteria\n";
    std::cout << "  export <format>         Export logs to different formats\n";
    std::cout << "  tail [file]             Follow log files in real-time\n\n";
    std::cout << "Examples:\n";
    std::cout << "  netlogai log parse network.log --parser cisco-ios\n";
    std::cout << "  netlogai log analyze errors.log --pattern \"BGP.*down\"\n";
    std::cout << "  netlogai log show --lines 50 --verbose\n";
    std::cout << "  netlogai log tail /var/log/network.log\n";
}

std::string LogCommands::get_default_log_directory() {
    std::filesystem::path home_dir;

#ifdef _WIN32
    if (const char* userprofile = std::getenv("USERPROFILE")) {
        home_dir = userprofile;
    } else {
        home_dir = "C:\\";
    }
    return (home_dir / ".netlogai" / "logs").string();
#else
    if (const char* home = std::getenv("HOME")) {
        home_dir = home;
    } else {
        home_dir = "/tmp";
    }
    return (home_dir / ".netlogai" / "logs").string();
#endif
}

bool LogCommands::is_valid_log_file(const std::string& path) {
    return std::filesystem::exists(path) && std::filesystem::is_regular_file(path);
}

void LogCommands::print_log_entry(const std::string& entry, bool verbose) {
    if (verbose) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::cout << "[" << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << "] ";
    }
    std::cout << entry << "\n";
}

} // namespace netlogai::commands