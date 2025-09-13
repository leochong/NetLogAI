#include "config_commands.hpp"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace netlogai::commands {

// Global configuration storage
static json g_config;
static std::string g_config_file;

void ConfigCommands::register_commands(cli::CommandLine& cli) {
    cli.register_command("config", [](const cli::CommandArgs&) {
        show_config_help();
        return 0;
    }, "Manage NetLogAI configuration");

    cli.register_subcommand("config", "get", get_config, "Get configuration value");
    cli.register_subcommand("config", "set", set_config, "Set configuration value");
    cli.register_subcommand("config", "list", list_config, "List all configuration settings");
    cli.register_subcommand("config", "reset", reset_config, "Reset configuration to defaults");
    cli.register_subcommand("config", "init", init_config, "Initialize configuration file");
}

int ConfigCommands::init_config(const cli::CommandArgs& args) {
    std::string config_file = get_config_file_path();

    if (std::filesystem::exists(config_file) && !args.has_flag("force")) {
        std::cerr << "Configuration file already exists: " << config_file << "\n";
        std::cerr << "Use --force to overwrite existing configuration\n";
        return 1;
    }

    try {
        // Create config directory if it doesn't exist
        std::filesystem::create_directories(std::filesystem::path(config_file).parent_path());

        // Initialize with default configuration
        json default_config = json::parse(get_default_config());

        std::ofstream file(config_file);
        file << default_config.dump(2) << std::endl;

        std::cout << "Configuration initialized: " << config_file << "\n";
        std::cout << "\nDefault settings:\n";
        std::cout << "=================\n";
        for (auto& [key, value] : default_config.items()) {
            std::cout << key << " = " << value << "\n";
        }

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: Failed to initialize configuration: " << e.what() << "\n";
        return 1;
    }
}

int ConfigCommands::get_config(const cli::CommandArgs& args) {
    if (args.arg_count() == 0) {
        std::cerr << "Error: No configuration key specified.\n";
        std::cerr << "Usage: netlogai config get <key>\n";
        return 1;
    }

    std::string key = args.get_arg(0);

    if (!load_config_file(get_config_file_path())) {
        std::cerr << "Error: Failed to load configuration\n";
        std::cerr << "Run 'netlogai config init' to initialize configuration\n";
        return 1;
    }

    std::string value = get_config_value(key);
    if (value.empty()) {
        std::cerr << "Configuration key not found: " << key << "\n";
        return 1;
    }

    std::cout << key << " = " << value << "\n";
    return 0;
}

int ConfigCommands::set_config(const cli::CommandArgs& args) {
    if (args.arg_count() < 2) {
        std::cerr << "Error: Key and value required.\n";
        std::cerr << "Usage: netlogai config set <key> <value>\n";
        return 1;
    }

    std::string key = args.get_arg(0);
    std::string value = args.get_arg(1);

    if (!load_config_file(get_config_file_path())) {
        std::cerr << "Configuration file not found. Initializing...\n";
        if (init_config(cli::CommandArgs{}) != 0) {
            return 1;
        }
        load_config_file(get_config_file_path());
    }

    if (!set_config_value(key, value)) {
        std::cerr << "Error: Failed to set configuration value\n";
        return 1;
    }

    if (!save_config_file(get_config_file_path())) {
        std::cerr << "Error: Failed to save configuration\n";
        return 1;
    }

    std::cout << "Configuration updated: " << key << " = " << value << "\n";
    return 0;
}

int ConfigCommands::list_config(const cli::CommandArgs& args) {
    if (!load_config_file(get_config_file_path())) {
        std::cerr << "Error: Configuration file not found\n";
        std::cerr << "Run 'netlogai config init' to initialize configuration\n";
        return 1;
    }

    std::cout << "NetLogAI Configuration\n";
    std::cout << "======================\n";
    std::cout << "Config file: " << get_config_file_path() << "\n\n";

    for (auto& [key, value] : g_config.items()) {
        std::cout << key << " = ";
        if (value.is_string()) {
            std::cout << value.get<std::string>();
        } else if (value.is_boolean()) {
            std::cout << (value.get<bool>() ? "true" : "false");
        } else if (value.is_number()) {
            std::cout << value.get<double>();
        } else {
            std::cout << value.dump();
        }
        std::cout << "\n";
    }

    return 0;
}

int ConfigCommands::reset_config(const cli::CommandArgs& args) {
    std::string key = args.get_arg(0);

    if (key.empty()) {
        // Reset entire configuration
        if (!args.has_flag("confirm")) {
            std::cerr << "This will reset ALL configuration to defaults.\n";
            std::cerr << "Use --confirm to proceed with full reset\n";
            return 1;
        }

        return init_config(cli::CommandArgs{{}, {{"force", "true"}}, {"force"}});
    } else {
        // Reset specific key
        std::cerr << "Resetting individual keys not yet implemented.\n";
        std::cerr << "Use 'netlogai config set " << key << " <default_value>' manually\n";
        return 1;
    }
}

void ConfigCommands::show_config_help() {
    std::cout << "NetLogAI Configuration Management\n";
    std::cout << "=================================\n\n";
    std::cout << "Usage: netlogai config <subcommand> [options]\n\n";
    std::cout << "Subcommands:\n";
    std::cout << "  init                    Initialize configuration file\n";
    std::cout << "  get <key>              Get configuration value\n";
    std::cout << "  set <key> <value>      Set configuration value\n";
    std::cout << "  list                   List all configuration settings\n";
    std::cout << "  reset [key]            Reset configuration to defaults\n\n";
    std::cout << "Configuration Keys:\n";
    std::cout << "  parsers.directory      Directory for custom parsers\n";
    std::cout << "  logs.directory         Directory for log files\n";
    std::cout << "  ai.provider            AI provider (anthropic, openai)\n";
    std::cout << "  ai.api_key             API key for AI provider\n";
    std::cout << "  output.format          Default output format (json, text)\n";
    std::cout << "  debug.enabled          Enable debug logging\n\n";
    std::cout << "Examples:\n";
    std::cout << "  netlogai config init\n";
    std::cout << "  netlogai config get ai.provider\n";
    std::cout << "  netlogai config set parsers.directory /custom/parsers\n";
    std::cout << "  netlogai config list\n";
}

std::string ConfigCommands::get_config_file_path() {
    if (!g_config_file.empty()) {
        return g_config_file;
    }

    std::filesystem::path config_dir;

#ifdef _WIN32
    if (const char* userprofile = std::getenv("USERPROFILE")) {
        config_dir = std::filesystem::path(userprofile) / ".netlogai";
    } else {
        config_dir = std::filesystem::path("C:\\") / ".netlogai";
    }
#else
    if (const char* home = std::getenv("HOME")) {
        config_dir = std::filesystem::path(home) / ".netlogai";
    } else {
        config_dir = std::filesystem::path("/tmp") / ".netlogai";
    }
#endif

    g_config_file = (config_dir / "config.json").string();
    return g_config_file;
}

std::string ConfigCommands::get_default_config() {
    return R"({
  "parsers": {
    "directory": "~/.netlogai/parsers",
    "auto_detect": true
  },
  "logs": {
    "directory": "~/.netlogai/logs",
    "max_file_size": "100MB",
    "retention_days": 30
  },
  "ai": {
    "provider": "none",
    "api_key": "",
    "model": "default"
  },
  "output": {
    "format": "json",
    "pretty_print": true,
    "color": true
  },
  "debug": {
    "enabled": false,
    "log_level": "info"
  },
  "network": {
    "timeout": 30,
    "retry_count": 3
  }
})";
}

bool ConfigCommands::load_config_file(const std::string& path) {
    try {
        if (!std::filesystem::exists(path)) {
            return false;
        }

        std::ifstream file(path);
        if (!file) {
            return false;
        }

        file >> g_config;
        return true;

    } catch (const std::exception&) {
        return false;
    }
}

bool ConfigCommands::save_config_file(const std::string& path) {
    try {
        std::ofstream file(path);
        if (!file) {
            return false;
        }

        file << g_config.dump(2) << std::endl;
        return true;

    } catch (const std::exception&) {
        return false;
    }
}

std::string ConfigCommands::get_config_value(const std::string& key) {
    try {
        // Support nested keys with dot notation (e.g., "ai.provider")
        json current = g_config;
        std::string remaining = key;

        while (!remaining.empty()) {
            size_t dot_pos = remaining.find('.');
            std::string part = (dot_pos == std::string::npos) ?
                              remaining : remaining.substr(0, dot_pos);

            if (!current.contains(part)) {
                return "";
            }

            current = current[part];

            if (dot_pos == std::string::npos) {
                break;
            }
            remaining = remaining.substr(dot_pos + 1);
        }

        if (current.is_string()) {
            return current.get<std::string>();
        } else if (current.is_boolean()) {
            return current.get<bool>() ? "true" : "false";
        } else if (current.is_number()) {
            return std::to_string(current.get<double>());
        } else {
            return current.dump();
        }

    } catch (const std::exception&) {
        return "";
    }
}

bool ConfigCommands::set_config_value(const std::string& key, const std::string& value) {
    try {
        // Support nested keys with dot notation
        json* current = &g_config;
        std::string remaining = key;

        while (true) {
            size_t dot_pos = remaining.find('.');
            if (dot_pos == std::string::npos) {
                // Last part - set the value
                (*current)[remaining] = value;
                break;
            }

            std::string part = remaining.substr(0, dot_pos);
            if (!current->contains(part) || !(*current)[part].is_object()) {
                (*current)[part] = json::object();
            }
            current = &((*current)[part]);
            remaining = remaining.substr(dot_pos + 1);
        }

        return true;

    } catch (const std::exception&) {
        return false;
    }
}

} // namespace netlogai::commands