#pragma once

#include "../cli/command_line.hpp"

namespace netlogai::commands {

class ParserCommands {
public:
    static void register_commands(cli::CommandLine& cli);
    
private:
    static int list_parsers(const cli::CommandArgs& args);
    static int install_parser(const cli::CommandArgs& args);
    static int test_parser(const cli::CommandArgs& args);
    static int validate_parser(const cli::CommandArgs& args);
    static int uninstall_parser(const cli::CommandArgs& args);
    static int show_parser_info(const cli::CommandArgs& args);
    
    // Helper functions
    static void show_parser_help();
    static std::string get_parsers_directory();
    static std::string get_default_test_logs_path();
    static bool copy_file(const std::string& source, const std::string& destination);
    static bool file_exists(const std::string& path);
    static std::vector<std::string> find_parser_files(const std::string& directory);
};

} // namespace netlogai::commands