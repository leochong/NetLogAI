#include "plugin_commands.hpp"
#include <iostream>
#include <iomanip>

namespace netlogai::commands {

// Static member initialization
std::shared_ptr<plugins::PluginManager> PluginCommands::plugin_manager_ = nullptr;
bool PluginCommands::is_initialized_ = false;

void PluginCommands::register_commands(cli::CommandLine& cli) {
    // Initialize plugin system
    initialize_plugin_system();

    // Plugin management commands
    cli.register_command("plugin", [](const cli::CommandArgs&) {
        show_plugin_help();
        return 0;
    }, "Plugin management and execution");

    cli.register_subcommand("plugin", "list", list_plugins, "List available plugins");
    cli.register_subcommand("plugin", "install", install_plugin, "Install a plugin from file");
    cli.register_subcommand("plugin", "uninstall", uninstall_plugin, "Uninstall a plugin");
    cli.register_subcommand("plugin", "load", load_plugin, "Load a plugin");
    cli.register_subcommand("plugin", "unload", unload_plugin, "Unload a plugin");
    cli.register_subcommand("plugin", "enable", enable_plugin, "Enable a plugin");
    cli.register_subcommand("plugin", "disable", disable_plugin, "Disable a plugin");
    cli.register_subcommand("plugin", "info", plugin_info, "Show plugin information");
    cli.register_subcommand("plugin", "status", plugin_status, "Show plugin status");
    cli.register_subcommand("plugin", "config", plugin_config, "Configure plugin settings");
    cli.register_subcommand("plugin", "exec", execute_plugin_command, "Execute plugin command");
    cli.register_subcommand("plugin", "test", test_plugin, "Test a plugin");
    cli.register_subcommand("plugin", "validate", validate_plugin, "Validate plugin compliance");

    // Specialized plugin commands
    cli.register_command("security", [](const cli::CommandArgs& args) {
        return security_scan(args);
    }, "Network security analysis using security plugin");

    cli.register_command("perf", [](const cli::CommandArgs& args) {
        return performance_report(args);
    }, "Performance monitoring using performance plugin");

    cli.register_command("topology", [](const cli::CommandArgs& args) {
        return topology_map(args);
    }, "Network topology mapping using topology plugin");
}

void PluginCommands::initialize_plugin_system() {
    if (is_initialized_) {
        return;
    }

    plugin_manager_ = std::make_shared<plugins::PluginManager>();

    plugins::PluginManager::PluginConfig config;
    config.auto_load_plugins = true;
    config.enable_sandbox = true;
    config.plugin_directories = {"examples/plugins", "third-party/plugins"};

    if (plugin_manager_->initialize(config)) {
        std::cout << "Plugin system initialized successfully\n";
        is_initialized_ = true;
    } else {
        std::cerr << "Failed to initialize plugin system\n";
    }
}

std::shared_ptr<plugins::PluginManager> PluginCommands::get_plugin_manager() {
    if (!is_initialized_) {
        initialize_plugin_system();
    }
    return plugin_manager_;
}

int PluginCommands::list_plugins(const cli::CommandArgs& args) {
    auto manager = get_plugin_manager();
    if (!manager) {
        std::cerr << "Plugin system not available\n";
        return 1;
    }

    bool show_loaded_only = args.has_flag("loaded");
    bool show_active_only = args.has_flag("active");

    std::cout << "NetLogAI Plugins\n";
    std::cout << "================\n\n";

    if (show_active_only) {
        auto active_plugins = manager->get_active_plugins();
        std::cout << "Active Plugins (" << active_plugins.size() << "):\n";
        print_plugin_list(active_plugins);
    } else if (show_loaded_only) {
        auto loaded_plugins = manager->get_loaded_plugins();
        std::cout << "Loaded Plugins (" << loaded_plugins.size() << "):\n";
        print_plugin_list(loaded_plugins);
    } else {
        auto available_plugins = manager->get_available_plugins();
        std::cout << "Available Plugins (" << available_plugins.size() << "):\n";
        print_plugin_list(available_plugins);

        auto loaded_plugins = manager->get_loaded_plugins();
        if (!loaded_plugins.empty()) {
            std::cout << "\nLoaded Plugins (" << loaded_plugins.size() << "):\n";
            print_plugin_list(loaded_plugins);
        }
    }

    return 0;
}

void PluginCommands::print_plugin_list(const std::vector<std::string>& plugin_ids) {
    auto manager = get_plugin_manager();
    if (!manager) return;

    for (const auto& plugin_id : plugin_ids) {
        auto info = manager->get_plugin_info(plugin_id);
        auto status = manager->get_plugin_status(plugin_id);

        std::cout << "  • " << plugin_id;
        if (!info.version.empty()) {
            std::cout << " v" << info.version;
        }
        std::cout << " [" << status << "]";
        if (!info.description.empty()) {
            std::cout << " - " << info.description;
        }
        std::cout << "\n";
    }
}

int PluginCommands::plugin_info(const cli::CommandArgs& args) {
    if (args.args.empty()) {
        std::cerr << "Usage: netlogai plugin info <plugin-name>\n";
        return 1;
    }

    std::string plugin_name = args.args[0];
    auto manager = get_plugin_manager();
    if (!manager) {
        std::cerr << "Plugin system not available\n";
        return 1;
    }

    auto info = manager->get_plugin_info(plugin_name);
    if (info.name.empty()) {
        std::cerr << "Plugin not found: " << plugin_name << "\n";
        return 1;
    }

    print_plugin_info(info);
    return 0;
}

void PluginCommands::print_plugin_info(const plugins::PluginManifest& manifest) {
    std::cout << "Plugin Information\n";
    std::cout << "==================\n\n";

    std::cout << std::left;
    std::cout << std::setw(15) << "Name:" << manifest.name << "\n";
    std::cout << std::setw(15) << "Version:" << manifest.version << "\n";
    std::cout << std::setw(15) << "Author:" << manifest.author << "\n";
    std::cout << std::setw(15) << "Type:" << manifest.plugin_type << "\n";
    std::cout << std::setw(15) << "API Version:" << manifest.api_version << "\n";

    if (!manifest.description.empty()) {
        std::cout << std::setw(15) << "Description:" << manifest.description << "\n";
    }

    if (!manifest.capabilities.empty()) {
        std::cout << std::setw(15) << "Capabilities:";
        for (size_t i = 0; i < manifest.capabilities.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << manifest.capabilities[i];
        }
        std::cout << "\n";
    }

    if (!manifest.dependencies.empty()) {
        std::cout << std::setw(15) << "Dependencies:";
        for (size_t i = 0; i < manifest.dependencies.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << manifest.dependencies[i];
        }
        std::cout << "\n";
    }
}

int PluginCommands::load_plugin(const cli::CommandArgs& args) {
    if (args.args.empty()) {
        std::cerr << "Usage: netlogai plugin load <plugin-path>\n";
        return 1;
    }

    std::string plugin_path = args.args[0];
    auto manager = get_plugin_manager();
    if (!manager) {
        std::cerr << "Plugin system not available\n";
        return 1;
    }

    if (manager->load_plugin(plugin_path)) {
        std::cout << "Plugin loaded successfully: " << plugin_path << "\n";
        return 0;
    } else {
        std::cerr << "Failed to load plugin: " << plugin_path << "\n";
        return 1;
    }
}

int PluginCommands::unload_plugin(const cli::CommandArgs& args) {
    if (args.args.empty()) {
        std::cerr << "Usage: netlogai plugin unload <plugin-name>\n";
        return 1;
    }

    std::string plugin_name = args.args[0];
    auto manager = get_plugin_manager();
    if (!manager) {
        std::cerr << "Plugin system not available\n";
        return 1;
    }

    if (manager->unload_plugin(plugin_name)) {
        std::cout << "Plugin unloaded successfully: " << plugin_name << "\n";
        return 0;
    } else {
        std::cerr << "Failed to unload plugin: " << plugin_name << "\n";
        return 1;
    }
}

int PluginCommands::enable_plugin(const cli::CommandArgs& args) {
    if (args.args.empty()) {
        std::cerr << "Usage: netlogai plugin enable <plugin-name>\n";
        return 1;
    }

    std::string plugin_name = args.args[0];
    auto manager = get_plugin_manager();
    if (!manager) {
        std::cerr << "Plugin system not available\n";
        return 1;
    }

    if (manager->enable_plugin(plugin_name)) {
        std::cout << "Plugin enabled successfully: " << plugin_name << "\n";
        return 0;
    } else {
        std::cerr << "Failed to enable plugin: " << plugin_name << "\n";
        return 1;
    }
}

int PluginCommands::disable_plugin(const cli::CommandArgs& args) {
    if (args.args.empty()) {
        std::cerr << "Usage: netlogai plugin disable <plugin-name>\n";
        return 1;
    }

    std::string plugin_name = args.args[0];
    auto manager = get_plugin_manager();
    if (!manager) {
        std::cerr << "Plugin system not available\n";
        return 1;
    }

    if (manager->disable_plugin(plugin_name)) {
        std::cout << "Plugin disabled successfully: " << plugin_name << "\n";
        return 0;
    } else {
        std::cerr << "Failed to disable plugin: " << plugin_name << "\n";
        return 1;
    }
}

int PluginCommands::plugin_status(const cli::CommandArgs& args) {
    auto manager = get_plugin_manager();
    if (!manager) {
        std::cerr << "Plugin system not available\n";
        return 1;
    }

    if (!args.args.empty()) {
        // Show status for specific plugin
        std::string plugin_name = args.args[0];
        auto status = manager->get_plugin_status(plugin_name);
        auto info = manager->get_plugin_info(plugin_name);

        std::cout << "Plugin Status: " << plugin_name << "\n";
        std::cout << std::string(20 + plugin_name.length(), '=') << "\n";
        std::cout << "Status: " << status << "\n";

        if (!info.name.empty()) {
            std::cout << "Version: " << info.version << "\n";
            std::cout << "Type: " << info.plugin_type << "\n";
        }
    } else {
        // Show status for all plugins
        auto loaded_plugins = manager->get_loaded_plugins();

        std::cout << "Plugin System Status\n";
        std::cout << "====================\n";
        std::cout << "Total loaded plugins: " << loaded_plugins.size() << "\n\n";

        for (const auto& plugin_id : loaded_plugins) {
            auto status = manager->get_plugin_status(plugin_id);
            std::cout << "  " << plugin_id << ": " << status << "\n";
        }
    }

    return 0;
}

int PluginCommands::execute_plugin_command(const cli::CommandArgs& args) {
    if (args.args.size() < 2) {
        std::cerr << "Usage: netlogai plugin exec <plugin-name> <command> [parameters...]\n";
        return 1;
    }

    std::string plugin_name = args.args[0];
    std::string command = args.args[1];

    auto manager = get_plugin_manager();
    if (!manager) {
        std::cerr << "Plugin system not available\n";
        return 1;
    }

    // Build parameters map
    std::map<std::string, std::string> parameters;
    for (size_t i = 2; i < args.args.size(); i += 2) {
        if (i + 1 < args.args.size()) {
            parameters[args.args[i]] = args.args[i + 1];
        }
    }

    auto result = manager->execute_plugin_command(plugin_name, command, parameters);

    if (result.success) {
        std::cout << result.message << "\n";

        // Print any additional data
        for (const auto& [key, value] : result.data) {
            std::cout << key << ": " << value << "\n";
        }

        return 0;
    } else {
        std::cerr << "Plugin command failed: " << result.message << "\n";
        for (const auto& error : result.errors) {
            std::cerr << "Error: " << error << "\n";
        }
        return 1;
    }
}

int PluginCommands::security_scan(const cli::CommandArgs& args) {
    auto manager = get_plugin_manager();
    if (!manager) {
        std::cerr << "Plugin system not available\n";
        return 1;
    }

    std::map<std::string, std::string> parameters;

    if (args.has_flag("reset")) {
        parameters["reset"] = "true";
    }

    auto result = manager->execute_plugin_command("security_plugin", "threat_report", parameters);

    if (result.success) {
        std::cout << "Security Analysis Report\n";
        std::cout << "========================\n";
        std::cout << result.message << "\n";
        return 0;
    } else {
        std::cerr << "Security scan failed: " << result.message << "\n";
        return 1;
    }
}

int PluginCommands::performance_report(const cli::CommandArgs& args) {
    auto manager = get_plugin_manager();
    if (!manager) {
        std::cerr << "Plugin system not available\n";
        return 1;
    }

    std::string device_id = args.get_option("device", "");

    std::map<std::string, std::string> parameters;
    if (!device_id.empty()) {
        parameters["device_id"] = device_id;
    }

    std::string command = args.has_flag("bandwidth") ? "bandwidth_report" : "performance_report";

    auto result = manager->execute_plugin_command("performance_plugin", command, parameters);

    if (result.success) {
        std::cout << "Performance Report\n";
        std::cout << "==================\n";
        std::cout << result.message << "\n";
        return 0;
    } else {
        std::cerr << "Performance report failed: " << result.message << "\n";
        return 1;
    }
}

int PluginCommands::topology_map(const cli::CommandArgs& args) {
    auto manager = get_plugin_manager();
    if (!manager) {
        std::cerr << "Plugin system not available\n";
        return 1;
    }

    std::string command = "topology_status";

    if (args.has_flag("diagram")) {
        command = "topology_diagram";
    } else if (args.has_flag("discover")) {
        command = "discover_devices";
    }

    std::map<std::string, std::string> parameters;

    auto result = manager->execute_plugin_command("topology_plugin", command, parameters);

    if (result.success) {
        std::cout << "Network Topology\n";
        std::cout << "================\n";
        std::cout << result.message << "\n";

        // Show topology diagram if requested
        if (command == "topology_diagram" && result.data.find("ascii_diagram") != result.data.end()) {
            std::cout << "\n" << result.data.at("ascii_diagram") << "\n";
        }

        return 0;
    } else {
        std::cerr << "Topology mapping failed: " << result.message << "\n";
        return 1;
    }
}

int PluginCommands::plugin_config(const cli::CommandArgs& args) {
    if (args.args.size() < 2) {
        std::cerr << "Usage: netlogai plugin config <plugin-name> <key> [value]\n";
        return 1;
    }

    std::string plugin_name = args.args[0];
    std::string config_key = args.args[1];
    std::string config_value = (args.args.size() > 2) ? args.args[2] : "";

    auto manager = get_plugin_manager();
    if (!manager) {
        std::cerr << "Plugin system not available\n";
        return 1;
    }

    if (!config_value.empty()) {
        // Set configuration value
        std::map<std::string, std::string> config;
        config[config_key] = config_value;

        if (manager->configure_plugin(plugin_name, config)) {
            std::cout << "Plugin configuration updated: " << plugin_name
                      << "." << config_key << " = " << config_value << "\n";
            return 0;
        } else {
            std::cerr << "Failed to update plugin configuration\n";
            return 1;
        }
    } else {
        // Get configuration value
        auto config = manager->get_plugin_config(plugin_name);
        auto it = config.find(config_key);

        if (it != config.end()) {
            std::cout << plugin_name << "." << config_key << " = " << it->second << "\n";
            return 0;
        } else {
            std::cerr << "Configuration key not found: " << config_key << "\n";
            return 1;
        }
    }
}

void PluginCommands::show_plugin_help() {
    std::cout << "NetLogAI Plugin System\n";
    std::cout << "======================\n\n";

    std::cout << "Plugin Management:\n";
    std::cout << "  plugin list [--loaded|--active]  List available plugins\n";
    std::cout << "  plugin install <path> [--verify] Install a plugin from file\n";
    std::cout << "  plugin uninstall <name>          Uninstall a plugin\n";
    std::cout << "  plugin load <path>               Load a plugin from file\n";
    std::cout << "  plugin unload <name>             Unload a plugin\n";
    std::cout << "  plugin enable <name>             Enable a plugin\n";
    std::cout << "  plugin disable <name>            Disable a plugin\n";
    std::cout << "  plugin info <name>               Show plugin information\n";
    std::cout << "  plugin status [name]             Show plugin status\n";
    std::cout << "  plugin config <name> <key> [val] Get/set plugin configuration\n";
    std::cout << "  plugin exec <name> <cmd> [args]  Execute plugin command\n";
    std::cout << "  plugin test <name|path>          Test plugin functionality\n";
    std::cout << "  plugin validate <path>           Validate plugin compliance\n\n";

    std::cout << "Specialized Plugin Commands:\n";
    std::cout << "  security [--reset]               Run security analysis\n";
    std::cout << "  perf [--bandwidth] [--device X]  Generate performance report\n";
    std::cout << "  topology [--diagram|--discover]  Show network topology\n\n";

    std::cout << "Examples:\n";
    std::cout << "  netlogai plugin list\n";
    std::cout << "  netlogai plugin info security_plugin\n";
    std::cout << "  netlogai security\n";
    std::cout << "  netlogai perf --device Router1\n";
    std::cout << "  netlogai topology --diagram\n";
}

// New plugin management functions
int PluginCommands::install_plugin(const cli::CommandArgs& args) {
    if (args.args.empty()) {
        std::cerr << "Error: Plugin path required\n";
        std::cerr << "Usage: netlogai plugin install <path> [--verify]\n";
        return 1;
    }

    std::string plugin_path = args.args[0];
    bool verify = args.has_flag("verify");

    std::cout << "Installing plugin: " << plugin_path << std::endl;

    if (verify) {
        std::cout << "Running security verification...\n";

        // Use the testing framework to validate the plugin
        testing::PluginValidator validator;
        auto report = validator.validate_plugin(plugin_path, testing::PluginValidator::ValidationLevel::STRICT);

        if (!report.is_valid) {
            std::cerr << "Plugin validation failed:\n";
            for (const auto& issue : report.compliance_issues) {
                std::cerr << "  - " << issue << "\n";
            }
            for (const auto& warning : report.security_warnings) {
                std::cerr << "  - SECURITY: " << warning << "\n";
            }
            return 1;
        }

        std::cout << "Plugin validation passed (score: " << report.overall_score << "/100)\n";
    }

    // Load the plugin to verify it works
    if (!plugin_manager_) {
        std::cerr << "Error: Plugin manager not initialized\n";
        return 1;
    }

    try {
        // Check if plugin file exists
        if (!std::filesystem::exists(plugin_path)) {
            std::cerr << "Error: Plugin file not found: " << plugin_path << std::endl;
            return 1;
        }

        // Determine installation directory (third-party vs official)
        std::string install_dir = verify ? "plugins" : "third-party/plugins";

        // Extract plugin name from path
        std::filesystem::path path_obj(plugin_path);
        std::string plugin_name = path_obj.stem().string();

        // Create target directory
        std::string target_dir = install_dir + "/" + plugin_name;
        std::filesystem::create_directories(target_dir);

        // Copy plugin file
        std::string target_path = target_dir + "/" + path_obj.filename().string();
        std::filesystem::copy_file(plugin_path, target_path, std::filesystem::copy_options::overwrite_existing);

        // Copy plugin manifest if it exists
        std::string manifest_source = path_obj.parent_path().string() + "/plugin.json";
        if (std::filesystem::exists(manifest_source)) {
            std::string manifest_target = target_dir + "/plugin.json";
            std::filesystem::copy_file(manifest_source, manifest_target, std::filesystem::copy_options::overwrite_existing);
        }

        // Try to load the plugin to verify installation
        if (plugin_manager_->load_plugin(target_path)) {
            std::cout << "Plugin installed successfully: " << plugin_name << std::endl;
            std::cout << "Installation location: " << target_dir << std::endl;
            return 0;
        } else {
            std::cerr << "Error: Failed to load installed plugin\n";
            // Clean up on failure
            std::filesystem::remove_all(target_dir);
            return 1;
        }

    } catch (const std::exception& e) {
        std::cerr << "Error installing plugin: " << e.what() << std::endl;
        return 1;
    }
}

int PluginCommands::uninstall_plugin(const cli::CommandArgs& args) {
    if (args.args.empty()) {
        std::cerr << "Error: Plugin name required\n";
        std::cerr << "Usage: netlogai plugin uninstall <name>\n";
        return 1;
    }

    std::string plugin_name = args.args[0];
    std::cout << "Uninstalling plugin: " << plugin_name << std::endl;

    if (!plugin_manager_) {
        std::cerr << "Error: Plugin manager not initialized\n";
        return 1;
    }

    try {
        // First unload the plugin if it's loaded
        plugin_manager_->unload_plugin(plugin_name);

        // Find and remove plugin directory
        std::vector<std::string> search_dirs = {"plugins/" + plugin_name, "third-party/plugins/" + plugin_name};

        bool found = false;
        for (const auto& dir : search_dirs) {
            if (std::filesystem::exists(dir)) {
                std::filesystem::remove_all(dir);
                std::cout << "Removed plugin directory: " << dir << std::endl;
                found = true;
                break;
            }
        }

        if (!found) {
            std::cerr << "Error: Plugin not found: " << plugin_name << std::endl;
            return 1;
        }

        std::cout << "Plugin uninstalled successfully: " << plugin_name << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error uninstalling plugin: " << e.what() << std::endl;
        return 1;
    }
}

int PluginCommands::test_plugin(const cli::CommandArgs& args) {
    if (args.args.empty()) {
        std::cerr << "Error: Plugin name or path required\n";
        std::cerr << "Usage: netlogai plugin test <name|path>\n";
        return 1;
    }

    std::string plugin_arg = args.args[0];
    std::cout << "Testing plugin: " << plugin_arg << std::endl;

    try {
        testing::PluginTestHarness test_harness;
        test_harness.setup_test_environment();

        // Determine if it's a path or name
        std::string plugin_path = plugin_arg;
        if (!std::filesystem::exists(plugin_arg)) {
            // Try to find by name in known directories
            std::vector<std::string> search_paths = {
                "plugins/" + plugin_arg + "/" + plugin_arg + ".dll",
                "third-party/plugins/" + plugin_arg + "/" + plugin_arg + ".dll",
                "build/plugins/" + plugin_arg + "/" + plugin_arg + ".dll"
            };

            bool found = false;
            for (const auto& path : search_paths) {
                if (std::filesystem::exists(path)) {
                    plugin_path = path;
                    found = true;
                    break;
                }
            }

            if (!found) {
                std::cerr << "Error: Plugin not found: " << plugin_arg << std::endl;
                return 1;
            }
        }

        if (test_harness.load_plugin_for_testing(plugin_path)) {
            auto results = test_harness.run_comprehensive_tests();

            // Display results
            std::cout << "\n=== Test Results ===\n";
            int passed = 0, failed = 0;
            for (const auto& result : results) {
                std::cout << result.test_name << ": " << (result.passed ? "PASSED" : "FAILED") << "\n";
                if (!result.message.empty()) {
                    std::cout << "  " << result.message << "\n";
                }
                for (const auto& warning : result.warnings) {
                    std::cout << "  WARNING: " << warning << "\n";
                }
                for (const auto& error : result.errors) {
                    std::cout << "  ERROR: " << error << "\n";
                }

                if (result.passed) passed++;
                else failed++;
            }

            std::cout << "\nSummary: " << passed << " passed, " << failed << " failed\n";

            // Generate detailed report
            test_harness.generate_test_report(plugin_arg, results);

            test_harness.cleanup_test_environment();
            return failed == 0 ? 0 : 1;
        } else {
            std::cerr << "Error: Failed to load plugin for testing\n";
            test_harness.cleanup_test_environment();
            return 1;
        }

    } catch (const std::exception& e) {
        std::cerr << "Error testing plugin: " << e.what() << std::endl;
        return 1;
    }
}

int PluginCommands::validate_plugin(const cli::CommandArgs& args) {
    if (args.args.empty()) {
        std::cerr << "Error: Plugin path required\n";
        std::cerr << "Usage: netlogai plugin validate <path> [--level=basic|standard|strict]\n";
        return 1;
    }

    std::string plugin_path = args.args[0];
    std::string level_str = args.get_option("level", "standard");

    testing::PluginValidator::ValidationLevel level = testing::PluginValidator::ValidationLevel::STANDARD;
    if (level_str == "basic") {
        level = testing::PluginValidator::ValidationLevel::BASIC;
    } else if (level_str == "strict") {
        level = testing::PluginValidator::ValidationLevel::STRICT;
    }

    std::cout << "Validating plugin: " << plugin_path << std::endl;
    std::cout << "Validation level: " << level_str << std::endl;

    try {
        testing::PluginValidator validator;
        auto report = validator.validate_plugin(plugin_path, level);

        std::cout << "\n=== Validation Results ===\n";
        std::cout << "Status: " << (report.is_valid ? "VALID" : "INVALID") << "\n";
        std::cout << "Overall Score: " << report.overall_score << "/100\n";

        if (!report.compliance_issues.empty()) {
            std::cout << "\nCompliance Issues:\n";
            for (const auto& issue : report.compliance_issues) {
                std::cout << "  - " << issue << "\n";
            }
        }

        if (!report.security_warnings.empty()) {
            std::cout << "\nSecurity Warnings:\n";
            for (const auto& warning : report.security_warnings) {
                std::cout << "  - " << warning << "\n";
            }
        }

        if (!report.performance_issues.empty()) {
            std::cout << "\nPerformance Issues:\n";
            for (const auto& issue : report.performance_issues) {
                std::cout << "  - " << issue << "\n";
            }
        }

        return report.is_valid ? 0 : 1;

    } catch (const std::exception& e) {
        std::cerr << "Error validating plugin: " << e.what() << std::endl;
        return 1;
    }
}

} // namespace netlogai::commands