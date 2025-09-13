#include "libnetlog/parsers/lua_parser.hpp"

#ifdef LIBNETLOG_ENABLE_LUA

namespace libnetlog {

LuaParser::LuaParser(const std::string& script_path) 
    : lua_engine_(std::make_unique<LuaEngine>())
    , script_path_(script_path)
    , is_from_file_(true) {
    
    if (!lua_engine_->load_script(script_path)) {
        // Keep the engine but mark it as invalid
        // Error can be retrieved via get_last_error()
    }
}

LuaParser::LuaParser(const std::string& script_content, const std::string& script_name)
    : lua_engine_(std::make_unique<LuaEngine>())
    , script_content_(script_content)
    , script_name_(script_name)
    , is_from_file_(false) {
    
    if (!lua_engine_->load_script_from_string(script_content, script_name)) {
        // Keep the engine but mark it as invalid
        // Error can be retrieved via get_last_error()
    }
}

std::optional<LogEntry> LuaParser::parse(const std::string& raw_message) {
    if (!is_valid()) {
        return std::nullopt;
    }
    
    return lua_engine_->parse(raw_message);
}

bool LuaParser::can_parse(const std::string& raw_message) const {
    if (!is_valid()) {
        return false;
    }
    
    return lua_engine_->can_parse(raw_message);
}

DeviceType LuaParser::get_device_type() const {
    if (!is_valid()) {
        return DeviceType::Unknown;
    }
    
    return lua_engine_->get_device_type();
}

std::string LuaParser::get_parser_name() const {
    if (!is_valid()) {
        return is_from_file_ ? script_path_ : script_name_;
    }
    
    return lua_engine_->get_parser_name();
}

std::string LuaParser::get_version() const {
    if (!is_valid()) {
        return "1.0.0";
    }
    
    return lua_engine_->get_version();
}

std::vector<std::string> LuaParser::get_supported_patterns() const {
    if (!is_valid()) {
        return {};
    }
    
    return lua_engine_->get_supported_patterns();
}

std::string LuaParser::get_last_error() const {
    if (!lua_engine_) {
        return "Lua engine not initialized";
    }
    
    return lua_engine_->get_last_error();
}

bool LuaParser::reload_script() {
    if (!lua_engine_) {
        return false;
    }
    
    lua_engine_->reset();
    
    if (is_from_file_) {
        return lua_engine_->load_script(script_path_);
    } else {
        return lua_engine_->load_script_from_string(script_content_, script_name_);
    }
}

} // namespace libnetlog

#endif // LIBNETLOG_ENABLE_LUA