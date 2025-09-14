#include "cli/command_line.hpp"
#include "commands/parser_commands.hpp"
#include "commands/log_commands.hpp"
#include "commands/config_commands.hpp"
#include "commands/device_commands.hpp"
#include "commands/ai_commands.hpp"
#include "commands/plugin_commands.hpp"
#include "analytics/git_style_commands.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    try {
        netlogai::cli::CommandLine cli;

        // Register all command modules
        netlogai::commands::ParserCommands::register_commands(cli);
        netlogai::commands::LogCommands::register_commands(cli);
        netlogai::commands::ConfigCommands::register_commands(cli);
        netlogai::commands::DeviceCommands::register_commands(cli);
        netlogai::commands::AICommands::register_commands(cli);
        netlogai::commands::PluginCommands::register_commands(cli);

        // Register advanced analytics commands (git-style interface)
        netlogai::analytics::GitStyleCommands::register_commands(cli);
        
        cli.register_command("status", [](const netlogai::cli::CommandArgs&) {
            std::cout << "NetLogAI System Status\n";
            std::cout << "======================\n";
            std::cout << "Core: Online\n";
            std::cout << "Parser Engine: Available\n";
#ifdef LIBNETLOG_ENABLE_LUA
            std::cout << "Lua Scripting: Enabled\n";
#else
            std::cout << "Lua Scripting: Disabled\n";
#endif
#ifdef ENABLE_AI_INTEGRATION
            std::cout << "AI Integration: Available\n";
#else
            std::cout << "AI Integration: Disabled\n";
#endif
            return 0;
        }, "Show system status and capabilities");
        
        return cli.execute(argc, argv);
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown fatal error occurred" << std::endl;
        return 1;
    }
}