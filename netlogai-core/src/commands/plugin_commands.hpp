#pragma once

#include "../cli/command_line.hpp"
#include "plugins/plugin_manager.hpp"
#include "testing/plugin_test_framework.hpp"
#include <memory>
#include <filesystem>

namespace netlogai::commands {

class PluginCommands {
public:
    static void register_commands(cli::CommandLine& cli);

private:
    // Plugin management commands
    static int list_plugins(const cli::CommandArgs& args);
    static int install_plugin(const cli::CommandArgs& args);
    static int uninstall_plugin(const cli::CommandArgs& args);
    static int load_plugin(const cli::CommandArgs& args);
    static int unload_plugin(const cli::CommandArgs& args);
    static int enable_plugin(const cli::CommandArgs& args);
    static int disable_plugin(const cli::CommandArgs& args);
    static int plugin_info(const cli::CommandArgs& args);
    static int plugin_status(const cli::CommandArgs& args);
    static int test_plugin(const cli::CommandArgs& args);
    static int validate_plugin(const cli::CommandArgs& args);

    // Plugin execution commands
    static int execute_plugin_command(const cli::CommandArgs& args);
    static int security_scan(const cli::CommandArgs& args);
    static int performance_report(const cli::CommandArgs& args);
    static int topology_map(const cli::CommandArgs& args);

    // Plugin configuration
    static int configure_plugin(const cli::CommandArgs& args);
    static int plugin_config(const cli::CommandArgs& args);

    // Helper functions
    static void show_plugin_help();
    static std::shared_ptr<plugins::PluginManager> get_plugin_manager();
    static void initialize_plugin_system();
    static void print_plugin_list(const std::vector<std::string>& plugin_ids);
    static void print_plugin_info(const plugins::PluginManifest& manifest);

    // Plugin manager instance
    static std::shared_ptr<plugins::PluginManager> plugin_manager_;
    static bool is_initialized_;
};

} // namespace netlogai::commands