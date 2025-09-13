#include "command_line.hpp"
#include <iostream>
#include <algorithm>
#include <sstream>

namespace netlogai::cli {

bool CommandArgs::has_flag(const std::string& flag) const {
    return std::find(flags.begin(), flags.end(), flag) != flags.end();
}

std::string CommandArgs::get_option(const std::string& key, const std::string& default_value) const {
    auto it = options.find(key);
    return (it != options.end()) ? it->second : default_value;
}

std::string CommandArgs::get_arg(size_t index, const std::string& default_value) const {
    return (index < args.size()) ? args[index] : default_value;
}

CommandLine::CommandLine() {
    // Register built-in commands
    register_command("help", [this](const CommandArgs& args) {
        if (args.arg_count() > 0) {
            show_command_help(args.get_arg(0));
        } else {
            show_help();
        }
        return 0;
    }, "Show help information");
    
    register_command("version", [this](const CommandArgs&) {
        show_version();
        return 0;
    }, "Show version information");
}

void CommandLine::register_command(const std::string& name, CommandHandler handler, const std::string& description) {
    commands_[name] = {name, description, handler, {}};
}

void CommandLine::register_subcommand(const std::string& parent, const std::string& name, CommandHandler handler, const std::string& description) {
    auto& parent_cmd = commands_[parent];
    parent_cmd.subcommands[name] = {name, description, handler, {}};
}

int CommandLine::execute(int argc, char* argv[]) {
    if (argc < 2) {
        show_help();
        return 1;
    }
    
    std::string command_name = argv[1];
    
    // Handle special flags
    if (command_name == "--help" || command_name == "-h") {
        show_help();
        return 0;
    }
    
    if (command_name == "--version" || command_name == "-v") {
        show_version();
        return 0;
    }
    
    auto it = commands_.find(command_name);
    if (it == commands_.end()) {
        std::cerr << "Unknown command: " << command_name << std::endl;
        std::cerr << "Run 'netlogai help' for available commands." << std::endl;
        return 1;
    }
    
    const auto& command = it->second;
    
    // Check for subcommands
    if (argc >= 3 && !command.subcommands.empty()) {
        std::string subcommand_name = argv[2];
        auto sub_it = command.subcommands.find(subcommand_name);
        if (sub_it != command.subcommands.end()) {
            CommandArgs args = parse_args(argc, argv, 3);
            return sub_it->second.handler(args);
        }
    }
    
    // Execute main command
    CommandArgs args = parse_args(argc, argv, 2);
    return command.handler(args);
}

CommandArgs CommandLine::parse_args(int argc, char* argv[], size_t start_index) const {
    CommandArgs result;
    
    for (size_t i = start_index; i < static_cast<size_t>(argc); ++i) {
        std::string arg = argv[i];
        
        if (arg.length() >= 2 && arg.substr(0, 2) == "--") {
            // Long option
            size_t eq_pos = arg.find('=');
            if (eq_pos != std::string::npos) {
                std::string key = arg.substr(2, eq_pos - 2);
                std::string value = arg.substr(eq_pos + 1);
                result.options[key] = value;
            } else {
                std::string key = arg.substr(2);
                if (i + 1 < static_cast<size_t>(argc) && std::string(argv[i + 1]).length() > 0 && argv[i + 1][0] != '-') {
                    result.options[key] = argv[++i];
                } else {
                    result.flags.push_back(key);
                }
            }
        } else if (arg.length() > 1 && arg[0] == '-' && arg[1] != '-') {
            // Short option(s)
            for (size_t j = 1; j < arg.length(); ++j) {
                result.flags.push_back(std::string(1, arg[j]));
            }
        } else {
            // Regular argument
            result.args.push_back(arg);
        }
    }
    
    return result;
}

void CommandLine::show_help() const {
    std::cout << "NetLogAI Core v1.0.0 - Enterprise Network Log Analysis\n";
    std::cout << "Copyright (c) 2024 NetLogAI. All rights reserved.\n\n";
    std::cout << "Usage: netlogai <command> [subcommand] [options]\n\n";
    std::cout << "Available commands:\n";
    
    for (const auto& [name, command] : commands_) {
        std::cout << "  " << name;
        std::cout << std::string(15 - name.length(), ' ');
        std::cout << command.description << "\n";
        
        // Show subcommands if any
        if (!command.subcommands.empty()) {
            for (const auto& [sub_name, sub_command] : command.subcommands) {
                std::cout << "    " << name << " " << sub_name;
                std::cout << std::string(13 - name.length() - sub_name.length(), ' ');
                std::cout << sub_command.description << "\n";
            }
        }
    }
    
    std::cout << "\nGlobal options:\n";
    std::cout << "  --help, -h      Show help information\n";
    std::cout << "  --version, -v   Show version information\n";
    std::cout << "\nRun 'netlogai help <command>' for detailed command information.\n";
}

void CommandLine::show_command_help(const std::string& command) const {
    auto it = commands_.find(command);
    if (it == commands_.end()) {
        std::cerr << "Unknown command: " << command << std::endl;
        return;
    }
    
    const auto& cmd = it->second;
    std::cout << "Command: " << cmd.name << "\n";
    std::cout << "Description: " << cmd.description << "\n\n";
    
    if (!cmd.subcommands.empty()) {
        std::cout << "Subcommands:\n";
        for (const auto& [sub_name, sub_command] : cmd.subcommands) {
            std::cout << "  " << sub_name;
            std::cout << std::string(15 - sub_name.length(), ' ');
            std::cout << sub_command.description << "\n";
        }
    }
}

void CommandLine::show_version() const {
    std::cout << "NetLogAI Core v1.0.0\n";
    std::cout << "Build: Commercial\n";
    std::cout << "Platform: Windows x64\n";
    std::cout << "Lua Scripting: Enabled\n";
    std::cout << "AI Integration: Available\n";
}

} // namespace netlogai::cli