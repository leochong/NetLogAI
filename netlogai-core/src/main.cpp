#include "cli/command_line.hpp"
#include "commands/parser_commands.hpp"
#include "commands/log_commands.hpp"
#include "commands/config_commands.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    try {
        netlogai::cli::CommandLine cli;

        // Register all command modules
        netlogai::commands::ParserCommands::register_commands(cli);
        netlogai::commands::LogCommands::register_commands(cli);
        netlogai::commands::ConfigCommands::register_commands(cli);
        
        // Register additional placeholder commands for future implementation
        cli.register_command("fetch", [](const netlogai::cli::CommandArgs&) {
            std::cout << "Device log fetching not yet implemented.\n";
            std::cout << "Coming soon: automatic log collection from network devices.\n";
            return 0;
        }, "Fetch logs from network devices");
        
        cli.register_command("ask", [](const netlogai::cli::CommandArgs&) {
            std::cout << "AI-powered analysis not yet implemented.\n";
            std::cout << "Coming soon: natural language queries about your logs.\n";
            return 0;
        }, "AI-powered log analysis queries");
        
        cli.register_command("analyze", [](const netlogai::cli::CommandArgs&) {
            std::cout << "Advanced analysis not yet implemented.\n";
            std::cout << "Coming soon: pattern detection and correlation analysis.\n";
            return 0;
        }, "Advanced log analysis and pattern detection");
        
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