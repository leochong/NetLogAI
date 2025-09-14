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
    cli.register_subcommand("plugin", "load", load_plugin, "Load a plugin");
    cli.register_subcommand("plugin", "unload", unload_plugin, "Unload a plugin");
    cli.register_subcommand("plugin", "enable", enable_plugin, "Enable a plugin");
    cli.register_subcommand("plugin", "disable", disable_plugin, "Disable a plugin");
    cli.register_subcommand("plugin", "info", plugin_info, "Show plugin information");
    cli.register_subcommand("plugin", "status", plugin_status, "Show plugin status");
    cli.register_subcommand("plugin", "config", plugin_config, "Configure plugin settings");
    cli.register_subcommand("plugin", "exec", execute_plugin_command, "Execute plugin command");

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
    std::cout << "  plugin load <path>               Load a plugin from file\n";
    std::cout << "  plugin unload <name>             Unload a plugin\n";
    std::cout << "  plugin enable <name>             Enable a plugin\n";
    std::cout << "  plugin disable <name>            Disable a plugin\n";
    std::cout << "  plugin info <name>               Show plugin information\n";
    std::cout << "  plugin status [name]             Show plugin status\n";
    std::cout << "  plugin config <name> <key> [val] Get/set plugin configuration\n";
    std::cout << "  plugin exec <name> <cmd> [args]  Execute plugin command\n\n";

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

} // namespace netlogai::commands