#pragma once

#include "../cli/command_line.hpp"

namespace netlogai::commands {

class ConfigCommands {
public:
    static void register_commands(cli::CommandLine& cli);

private:
    static int get_config(const cli::CommandArgs& args);
    static int set_config(const cli::CommandArgs& args);
    static int list_config(const cli::CommandArgs& args);
    static int reset_config(const cli::CommandArgs& args);
    static int init_config(const cli::CommandArgs& args);

    // Helper functions
    static void show_config_help();
    static std::string get_config_file_path();
    static std::string get_default_config();
    static bool load_config_file(const std::string& path);
    static bool save_config_file(const std::string& path);
    static std::string get_config_value(const std::string& key);
    static bool set_config_value(const std::string& key, const std::string& value);
};

} // namespace netlogai::commands