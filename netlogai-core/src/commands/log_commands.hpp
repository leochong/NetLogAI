#pragma once

#include "../cli/command_line.hpp"

namespace netlogai::commands {

class LogCommands {
public:
    static void register_commands(cli::CommandLine& cli);

private:
    static int parse_logs(const cli::CommandArgs& args);
    static int analyze_logs(const cli::CommandArgs& args);
    static int show_logs(const cli::CommandArgs& args);
    static int filter_logs(const cli::CommandArgs& args);
    static int export_logs(const cli::CommandArgs& args);
    static int tail_logs(const cli::CommandArgs& args);

    // Helper functions
    static void show_log_help();
    static std::string get_default_log_directory();
    static bool is_valid_log_file(const std::string& path);
    static void print_log_entry(const std::string& entry, bool verbose = false);
};

} // namespace netlogai::commands