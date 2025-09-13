#pragma once

#include <string>
#include <vector>
#include <functional>
#include <map>

namespace netlogai::cli {

struct CommandArgs {
    std::vector<std::string> args;
    std::map<std::string, std::string> options;
    std::vector<std::string> flags;
    
    bool has_flag(const std::string& flag) const;
    std::string get_option(const std::string& key, const std::string& default_value = "") const;
    size_t arg_count() const { return args.size(); }
    std::string get_arg(size_t index, const std::string& default_value = "") const;
};

class CommandLine {
public:
    using CommandHandler = std::function<int(const CommandArgs&)>;
    
    CommandLine();
    
    void register_command(const std::string& name, CommandHandler handler, const std::string& description);
    void register_subcommand(const std::string& parent, const std::string& name, CommandHandler handler, const std::string& description);
    
    int execute(int argc, char* argv[]);
    void show_help() const;
    void show_version() const;

private:
    struct Command {
        std::string name;
        std::string description;
        CommandHandler handler;
        std::map<std::string, Command> subcommands;
    };
    
    std::map<std::string, Command> commands_;
    
    CommandArgs parse_args(int argc, char* argv[], size_t start_index = 1) const;
    void show_command_help(const std::string& command) const;
};

} // namespace netlogai::cli