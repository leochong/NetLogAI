#pragma once

#include "log_entry.hpp"
#include "device_types.hpp"
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <unordered_map>

#ifdef LIBNETLOG_ENABLE_LUA
extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}
#endif

namespace libnetlog {

#ifdef LIBNETLOG_ENABLE_LUA

/**
 * @brief Lua scripting engine for NetLog Parser DSL
 * 
 * This class provides a Lua-based domain-specific language for creating
 * custom network log parsers. It allows users to write parser scripts
 * that can handle device-specific log formats not covered by built-in parsers.
 */
class LuaEngine {
public:
    /**
     * @brief Constructor - initializes Lua state
     */
    LuaEngine();
    
    /**
     * @brief Destructor - cleans up Lua state
     */
    ~LuaEngine();

    // Non-copyable
    LuaEngine(const LuaEngine&) = delete;
    LuaEngine& operator=(const LuaEngine&) = delete;

    // Movable
    LuaEngine(LuaEngine&& other) noexcept;
    LuaEngine& operator=(LuaEngine&& other) noexcept;

    /**
     * @brief Load a parser script from file
     * 
     * @param script_path Path to the .nlp (NetLog Parser) script file
     * @return true if script loaded successfully, false otherwise
     */
    bool load_script(const std::string& script_path);

    /**
     * @brief Load a parser script from string
     * 
     * @param script_content The Lua script content
     * @param script_name Optional name for error reporting
     * @return true if script loaded successfully, false otherwise
     */
    bool load_script_from_string(const std::string& script_content, 
                                 const std::string& script_name = "inline_script");

    /**
     * @brief Parse a log message using the loaded script
     * 
     * @param raw_message The raw log message to parse
     * @return LogEntry if parsing successful, std::nullopt if parsing failed
     */
    std::optional<LogEntry> parse(const std::string& raw_message);

    /**
     * @brief Check if the loaded script can parse a given message
     * 
     * @param raw_message The raw log message to check
     * @return true if the script can likely handle the message, false otherwise
     */
    bool can_parse(const std::string& raw_message);

    /**
     * @brief Get the device type supported by the loaded script
     * 
     * @return DeviceType that this script is designed for
     */
    DeviceType get_device_type() const;

    /**
     * @brief Get the parser name from the script
     * 
     * @return String identifier for this parser
     */
    std::string get_parser_name() const;

    /**
     * @brief Get the script version
     * 
     * @return Version string for this parser script
     */
    std::string get_version() const;

    /**
     * @brief Get supported log format patterns from the script
     * 
     * @return Vector of regex patterns this script recognizes
     */
    std::vector<std::string> get_supported_patterns() const;

    /**
     * @brief Validate a script without loading it
     * 
     * @param script_path Path to the script file
     * @return true if script is valid, false otherwise
     */
    static bool validate_script(const std::string& script_path);

    /**
     * @brief Get the last error message
     * 
     * @return String describing the last error that occurred
     */
    std::string get_last_error() const { return last_error_; }

    /**
     * @brief Check if a script is currently loaded
     * 
     * @return true if a valid script is loaded, false otherwise
     */
    bool is_script_loaded() const { return script_loaded_; }

    /**
     * @brief Reset the engine state (unload current script)
     */
    void reset();

private:
    lua_State* L_;
    bool script_loaded_;
    std::string last_error_;
    std::string script_name_;

    // Internal helper methods
    bool initialize_lua_state();
    void cleanup_lua_state();
    bool register_api_functions();
    void set_error(const std::string& error);
    bool call_lua_function(const std::string& function_name, int num_args = 0, int num_results = 0);
    
    // API functions exposed to Lua scripts
    static int lua_create_log_entry(lua_State* L);
    static int lua_parse_timestamp(lua_State* L);
    static int lua_parse_severity(lua_State* L);
    static int lua_parse_device_type(lua_State* L);
    static int lua_log_debug(lua_State* L);
    static int lua_log_info(lua_State* L);
    static int lua_log_warn(lua_State* L);
    static int lua_log_error(lua_State* L);
};

/**
 * @brief Registry for managing multiple Lua parser scripts
 * 
 * This class manages a collection of Lua-based parsers and provides
 * a unified interface for parser discovery and selection.
 */
class LuaParserRegistry {
public:
    /**
     * @brief Load all parser scripts from a directory
     * 
     * @param parsers_dir Directory containing .nlp parser script files
     * @return Number of successfully loaded parsers
     */
    size_t load_parsers_from_directory(const std::string& parsers_dir);

    /**
     * @brief Register a single parser script
     * 
     * @param script_path Path to the parser script file
     * @param parser_name Optional custom name for the parser
     * @return true if parser registered successfully, false otherwise
     */
    bool register_parser(const std::string& script_path, 
                        const std::string& parser_name = "");

    /**
     * @brief Find the best parser for a given log message
     * 
     * @param raw_message The log message to analyze
     * @return Pointer to the best matching LuaEngine, nullptr if no match
     */
    LuaEngine* find_parser_for_message(const std::string& raw_message);

    /**
     * @brief Get a parser by name
     * 
     * @param parser_name Name of the parser to retrieve
     * @return Pointer to the LuaEngine, nullptr if not found
     */
    LuaEngine* get_parser(const std::string& parser_name);

    /**
     * @brief List all registered parser names
     * 
     * @return Vector of parser names
     */
    std::vector<std::string> list_parsers() const;

    /**
     * @brief Get parser information
     * 
     * @param parser_name Name of the parser
     * @return Map of parser metadata (name, version, device_type, etc.)
     */
    std::unordered_map<std::string, std::string> get_parser_info(const std::string& parser_name) const;

    /**
     * @brief Remove a parser from the registry
     * 
     * @param parser_name Name of the parser to remove
     * @return true if parser was removed, false if not found
     */
    bool unregister_parser(const std::string& parser_name);

    /**
     * @brief Clear all registered parsers
     */
    void clear();

    /**
     * @brief Get the number of registered parsers
     * 
     * @return Number of registered parsers
     */
    size_t size() const { return parsers_.size(); }

private:
    std::unordered_map<std::string, std::unique_ptr<LuaEngine>> parsers_;
};

#endif // LIBNETLOG_ENABLE_LUA

} // namespace libnetlog