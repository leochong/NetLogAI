#include "libnetlog/lua_engine.hpp"

#ifdef LIBNETLOG_ENABLE_LUA

#include "libnetlog/severity.hpp"
#include "libnetlog/device_types.hpp"
#include "libnetlog/utils/timestamp_parser.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <chrono>

namespace libnetlog {

LuaEngine::LuaEngine() : L_(nullptr), script_loaded_(false) {
    initialize_lua_state();
}

LuaEngine::~LuaEngine() {
    cleanup_lua_state();
}

LuaEngine::LuaEngine(LuaEngine&& other) noexcept 
    : L_(other.L_), script_loaded_(other.script_loaded_), 
      last_error_(std::move(other.last_error_)), script_name_(std::move(other.script_name_)) {
    other.L_ = nullptr;
    other.script_loaded_ = false;
}

LuaEngine& LuaEngine::operator=(LuaEngine&& other) noexcept {
    if (this != &other) {
        cleanup_lua_state();
        L_ = other.L_;
        script_loaded_ = other.script_loaded_;
        last_error_ = std::move(other.last_error_);
        script_name_ = std::move(other.script_name_);
        other.L_ = nullptr;
        other.script_loaded_ = false;
    }
    return *this;
}

bool LuaEngine::initialize_lua_state() {
    L_ = luaL_newstate();
    if (!L_) {
        set_error("Failed to create Lua state");
        return false;
    }

    // Open standard Lua libraries
    luaL_openlibs(L_);

    // Register our API functions
    return register_api_functions();
}

void LuaEngine::cleanup_lua_state() {
    if (L_) {
        lua_close(L_);
        L_ = nullptr;
    }
    script_loaded_ = false;
}

bool LuaEngine::register_api_functions() {
    // Create a global table for our API
    lua_newtable(L_);
    lua_setglobal(L_, "netlog");

    // Get the netlog table
    lua_getglobal(L_, "netlog");

    // Register API functions
    lua_pushcfunction(L_, lua_create_log_entry);
    lua_setfield(L_, -2, "create_log_entry");

    lua_pushcfunction(L_, lua_parse_timestamp);
    lua_setfield(L_, -2, "parse_timestamp");

    lua_pushcfunction(L_, lua_parse_severity);
    lua_setfield(L_, -2, "parse_severity");

    lua_pushcfunction(L_, lua_parse_device_type);
    lua_setfield(L_, -2, "parse_device_type");

    lua_pushcfunction(L_, lua_log_debug);
    lua_setfield(L_, -2, "log_debug");

    lua_pushcfunction(L_, lua_log_info);
    lua_setfield(L_, -2, "log_info");

    lua_pushcfunction(L_, lua_log_warn);
    lua_setfield(L_, -2, "log_warn");

    lua_pushcfunction(L_, lua_log_error);
    lua_setfield(L_, -2, "log_error");

    // Pop the netlog table
    lua_pop(L_, 1);

    return true;
}

bool LuaEngine::load_script(const std::string& script_path) {
    std::ifstream file(script_path);
    if (!file.is_open()) {
        set_error("Failed to open script file: " + script_path);
        return false;
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();

    script_name_ = std::filesystem::path(script_path).filename().string();
    return load_script_from_string(content, script_name_);
}

bool LuaEngine::load_script_from_string(const std::string& script_content, 
                                        const std::string& script_name) {
    if (!L_) {
        set_error("Lua state not initialized");
        return false;
    }

    script_name_ = script_name;

    // Load the script
    int result = luaL_loadbuffer(L_, script_content.c_str(), script_content.size(), script_name.c_str());
    if (result != LUA_OK) {
        std::string error = "Failed to load script: ";
        if (lua_isstring(L_, -1)) {
            error += lua_tostring(L_, -1);
        }
        lua_pop(L_, 1);
        set_error(error);
        return false;
    }

    // Execute the script to define functions
    result = lua_pcall(L_, 0, 0, 0);
    if (result != LUA_OK) {
        std::string error = "Failed to execute script: ";
        if (lua_isstring(L_, -1)) {
            error += lua_tostring(L_, -1);
        }
        lua_pop(L_, 1);
        set_error(error);
        return false;
    }

    // Verify required functions exist
    std::vector<std::string> required_functions = {"parse", "can_parse", "get_device_type", "get_parser_name"};
    for (const auto& func_name : required_functions) {
        lua_getglobal(L_, func_name.c_str());
        if (!lua_isfunction(L_, -1)) {
            lua_pop(L_, 1);
            set_error("Required function '" + func_name + "' not found in script");
            return false;
        }
        lua_pop(L_, 1);
    }

    script_loaded_ = true;
    return true;
}

std::optional<LogEntry> LuaEngine::parse(const std::string& raw_message) {
    if (!script_loaded_) {
        set_error("No script loaded");
        return std::nullopt;
    }

    // Call the parse function
    lua_getglobal(L_, "parse");
    lua_pushstring(L_, raw_message.c_str());

    if (lua_pcall(L_, 1, 1, 0) != LUA_OK) {
        std::string error = "Script parse function failed: ";
        if (lua_isstring(L_, -1)) {
            error += lua_tostring(L_, -1);
        }
        lua_pop(L_, 1);
        set_error(error);
        return std::nullopt;
    }

    // Check if result is nil (parse failed)
    if (lua_isnil(L_, -1)) {
        lua_pop(L_, 1);
        return std::nullopt;
    }

    // Result should be a table with log entry fields
    if (!lua_istable(L_, -1)) {
        lua_pop(L_, 1);
        set_error("Parse function must return a table or nil");
        return std::nullopt;
    }

    // Extract fields from the returned table
    LogEntry entry;

    // Get timestamp
    lua_getfield(L_, -1, "timestamp");
    if (lua_isnumber(L_, -1)) {
        auto timestamp = std::chrono::system_clock::from_time_t(static_cast<time_t>(lua_tonumber(L_, -1)));
        entry.set_timestamp(timestamp);
    } else if (lua_isstring(L_, -1)) {
        // TODO: Parse timestamp string
        entry.set_timestamp(std::chrono::system_clock::now());
    } else {
        entry.set_timestamp(std::chrono::system_clock::now());
    }
    lua_pop(L_, 1);

    // Get severity
    lua_getfield(L_, -1, "severity");
    if (lua_isstring(L_, -1)) {
        std::string severity_str = lua_tostring(L_, -1);
        auto severity = parse_severity(severity_str);
        entry.set_severity(severity);
    } else if (lua_isnumber(L_, -1)) {
        int severity_int = static_cast<int>(lua_tonumber(L_, -1));
        entry.set_severity(static_cast<Severity>(severity_int));
    }
    lua_pop(L_, 1);

    // Get message
    lua_getfield(L_, -1, "message");
    if (lua_isstring(L_, -1)) {
        entry.set_message(lua_tostring(L_, -1));
    }
    lua_pop(L_, 1);

    // Get facility
    lua_getfield(L_, -1, "facility");
    if (lua_isstring(L_, -1)) {
        entry.set_facility(lua_tostring(L_, -1));
    }
    lua_pop(L_, 1);

    // Get hostname
    lua_getfield(L_, -1, "hostname");
    if (lua_isstring(L_, -1)) {
        entry.set_hostname(lua_tostring(L_, -1));
    }
    lua_pop(L_, 1);

    // Get process_name
    lua_getfield(L_, -1, "process_name");
    if (lua_isstring(L_, -1)) {
        entry.set_process_name(lua_tostring(L_, -1));
    }
    lua_pop(L_, 1);

    // Get process_id
    lua_getfield(L_, -1, "process_id");
    if (lua_isnumber(L_, -1)) {
        entry.set_process_id(static_cast<uint32_t>(lua_tonumber(L_, -1)));
    }
    lua_pop(L_, 1);

    // Set device type
    entry.set_device_type(get_device_type());

    // Set raw message
    entry.set_raw_message(raw_message);

    // Get metadata table if present
    lua_getfield(L_, -1, "metadata");
    if (lua_istable(L_, -1)) {
        lua_pushnil(L_);
        while (lua_next(L_, -2) != 0) {
            if (lua_isstring(L_, -2) && lua_isstring(L_, -1)) {
                std::string key = lua_tostring(L_, -2);
                std::string value = lua_tostring(L_, -1);
                entry.add_metadata(key, value);
            }
            lua_pop(L_, 1);
        }
    }
    lua_pop(L_, 1);

    // Pop the result table
    lua_pop(L_, 1);

    return entry;
}

bool LuaEngine::can_parse(const std::string& raw_message) {
    if (!script_loaded_) {
        return false;
    }

    lua_getglobal(L_, "can_parse");
    lua_pushstring(L_, raw_message.c_str());

    if (lua_pcall(L_, 1, 1, 0) != LUA_OK) {
        lua_pop(L_, 1);
        return false;
    }

    bool result = lua_toboolean(L_, -1);
    lua_pop(L_, 1);
    return result;
}

DeviceType LuaEngine::get_device_type() const {
    if (!script_loaded_) {
        return DeviceType::Unknown;
    }

    lua_getglobal(L_, "get_device_type");
    if (lua_pcall(L_, 0, 1, 0) != LUA_OK) {
        lua_pop(L_, 1);
        return DeviceType::Unknown;
    }

    DeviceType device_type = DeviceType::Unknown;
    if (lua_isstring(L_, -1)) {
        std::string type_str = lua_tostring(L_, -1);
        device_type = parse_device_type(type_str);
    }
    lua_pop(L_, 1);
    return device_type;
}

std::string LuaEngine::get_parser_name() const {
    if (!script_loaded_) {
        return "";
    }

    lua_getglobal(L_, "get_parser_name");
    if (lua_pcall(L_, 0, 1, 0) != LUA_OK) {
        lua_pop(L_, 1);
        return script_name_;
    }

    std::string name;
    if (lua_isstring(L_, -1)) {
        name = lua_tostring(L_, -1);
    } else {
        name = script_name_;
    }
    lua_pop(L_, 1);
    return name;
}

std::string LuaEngine::get_version() const {
    if (!script_loaded_) {
        return "1.0.0";
    }

    lua_getglobal(L_, "get_version");
    if (lua_pcall(L_, 0, 1, 0) != LUA_OK) {
        lua_pop(L_, 1);
        return "1.0.0";
    }

    std::string version = "1.0.0";
    if (lua_isstring(L_, -1)) {
        version = lua_tostring(L_, -1);
    }
    lua_pop(L_, 1);
    return version;
}

std::vector<std::string> LuaEngine::get_supported_patterns() const {
    if (!script_loaded_) {
        return {};
    }

    lua_getglobal(L_, "get_supported_patterns");
    if (lua_pcall(L_, 0, 1, 0) != LUA_OK) {
        lua_pop(L_, 1);
        return {};
    }

    std::vector<std::string> patterns;
    if (lua_istable(L_, -1)) {
        lua_pushnil(L_);
        while (lua_next(L_, -2) != 0) {
            if (lua_isstring(L_, -1)) {
                patterns.emplace_back(lua_tostring(L_, -1));
            }
            lua_pop(L_, 1);
        }
    }
    lua_pop(L_, 1);
    return patterns;
}

bool LuaEngine::validate_script(const std::string& script_path) {
    LuaEngine temp_engine;
    return temp_engine.load_script(script_path);
}

void LuaEngine::reset() {
    cleanup_lua_state();
    initialize_lua_state();
}

void LuaEngine::set_error(const std::string& error) {
    last_error_ = error;
    std::cerr << "LuaEngine Error: " << error << std::endl;
}

bool LuaEngine::call_lua_function(const std::string& function_name, int num_args, int num_results) {
    lua_getglobal(L_, function_name.c_str());
    if (!lua_isfunction(L_, -1)) {
        lua_pop(L_, 1);
        return false;
    }

    // Arguments should already be on the stack
    return lua_pcall(L_, num_args, num_results, 0) == LUA_OK;
}

// Static API functions exposed to Lua scripts
int LuaEngine::lua_create_log_entry(lua_State* L) {
    // Return a new empty table that represents a log entry
    lua_newtable(L);
    return 1;
}

int LuaEngine::lua_parse_timestamp(lua_State* L) {
    if (lua_gettop(L) != 1 || !lua_isstring(L, 1)) {
        lua_pushnil(L);
        return 1;
    }

    std::string timestamp_str = lua_tostring(L, 1);

    // Use the enhanced timestamp parser
    auto timestamp = libnetlog::utils::TimestampParser::parse(timestamp_str);
    auto time_t_val = std::chrono::system_clock::to_time_t(timestamp);

    lua_pushnumber(L, static_cast<lua_Number>(time_t_val));
    return 1;
}

int LuaEngine::lua_parse_severity(lua_State* L) {
    if (lua_gettop(L) != 1 || !lua_isstring(L, 1)) {
        lua_pushinteger(L, static_cast<int>(Severity::Info));
        return 1;
    }

    std::string severity_str = lua_tostring(L, 1);
    auto severity = parse_severity(severity_str);
    lua_pushinteger(L, static_cast<int>(severity));
    return 1;
}

int LuaEngine::lua_parse_device_type(lua_State* L) {
    if (lua_gettop(L) != 1 || !lua_isstring(L, 1)) {
        lua_pushstring(L, "Unknown");
        return 1;
    }

    std::string device_str = lua_tostring(L, 1);
    auto device_type = parse_device_type(device_str);
    
    // Convert back to string for Lua
    switch (device_type) {
        case DeviceType::CiscoIOS: lua_pushstring(L, "CiscoIOS"); break;
        case DeviceType::CiscoNXOS: lua_pushstring(L, "CiscoNXOS"); break;
        case DeviceType::CiscoASA: lua_pushstring(L, "CiscoASA"); break;
        case DeviceType::GenericSyslog: lua_pushstring(L, "GenericSyslog"); break;
        default: lua_pushstring(L, "Unknown"); break;
    }
    return 1;
}

int LuaEngine::lua_log_debug(lua_State* L) {
    if (lua_gettop(L) >= 1 && lua_isstring(L, 1)) {
        std::cout << "[DEBUG] " << lua_tostring(L, 1) << std::endl;
    }
    return 0;
}

int LuaEngine::lua_log_info(lua_State* L) {
    if (lua_gettop(L) >= 1 && lua_isstring(L, 1)) {
        std::cout << "[INFO] " << lua_tostring(L, 1) << std::endl;
    }
    return 0;
}

int LuaEngine::lua_log_warn(lua_State* L) {
    if (lua_gettop(L) >= 1 && lua_isstring(L, 1)) {
        std::cout << "[WARN] " << lua_tostring(L, 1) << std::endl;
    }
    return 0;
}

int LuaEngine::lua_log_error(lua_State* L) {
    if (lua_gettop(L) >= 1 && lua_isstring(L, 1)) {
        std::cout << "[ERROR] " << lua_tostring(L, 1) << std::endl;
    }
    return 0;
}

// LuaParserRegistry implementation
size_t LuaParserRegistry::load_parsers_from_directory(const std::string& parsers_dir) {
    size_t loaded_count = 0;
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(parsers_dir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".nlp") {
                if (register_parser(entry.path().string())) {
                    loaded_count++;
                }
            }
        }
    } catch (const std::filesystem::filesystem_error& ex) {
        std::cerr << "Failed to scan parsers directory: " << ex.what() << std::endl;
    }
    
    return loaded_count;
}

bool LuaParserRegistry::register_parser(const std::string& script_path, const std::string& parser_name) {
    auto engine = std::make_unique<LuaEngine>();
    if (!engine->load_script(script_path)) {
        return false;
    }
    
    std::string name = parser_name.empty() ? engine->get_parser_name() : parser_name;
    parsers_[name] = std::move(engine);
    return true;
}

LuaEngine* LuaParserRegistry::find_parser_for_message(const std::string& raw_message) {
    for (auto& [name, engine] : parsers_) {
        if (engine->can_parse(raw_message)) {
            return engine.get();
        }
    }
    return nullptr;
}

LuaEngine* LuaParserRegistry::get_parser(const std::string& parser_name) {
    auto it = parsers_.find(parser_name);
    return (it != parsers_.end()) ? it->second.get() : nullptr;
}

std::vector<std::string> LuaParserRegistry::list_parsers() const {
    std::vector<std::string> names;
    names.reserve(parsers_.size());
    for (const auto& [name, engine] : parsers_) {
        names.push_back(name);
    }
    return names;
}

std::unordered_map<std::string, std::string> 
LuaParserRegistry::get_parser_info(const std::string& parser_name) const {
    auto it = parsers_.find(parser_name);
    if (it == parsers_.end()) {
        return {};
    }
    
    const auto& engine = it->second;
    std::unordered_map<std::string, std::string> info;
    info["name"] = engine->get_parser_name();
    info["version"] = engine->get_version();
    
    auto device_type = engine->get_device_type();
    switch (device_type) {
        case DeviceType::CiscoIOS: info["device_type"] = "CiscoIOS"; break;
        case DeviceType::CiscoNXOS: info["device_type"] = "CiscoNXOS"; break;
        case DeviceType::CiscoASA: info["device_type"] = "CiscoASA"; break;
        case DeviceType::GenericSyslog: info["device_type"] = "GenericSyslog"; break;
        default: info["device_type"] = "Unknown"; break;
    }
    
    return info;
}

bool LuaParserRegistry::unregister_parser(const std::string& parser_name) {
    return parsers_.erase(parser_name) > 0;
}

void LuaParserRegistry::clear() {
    parsers_.clear();
}

} // namespace libnetlog

#endif // LIBNETLOG_ENABLE_LUA