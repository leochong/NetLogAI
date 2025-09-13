#pragma once

#include "../parsers/base_parser.hpp"
#include "../lua_engine.hpp"
#include <memory>

namespace libnetlog {

#ifdef LIBNETLOG_ENABLE_LUA

/**
 * @brief Lua-based parser that implements the BaseParser interface
 * 
 * This class wraps a LuaEngine to provide a parser that can be used
 * interchangeably with built-in C++ parsers through the BaseParser interface.
 */
class LuaParser : public BaseParser {
public:
    /**
     * @brief Constructor - creates parser from script file
     * 
     * @param script_path Path to the .nlp parser script file
     */
    explicit LuaParser(const std::string& script_path);

    /**
     * @brief Constructor - creates parser from script content
     * 
     * @param script_content The Lua script content
     * @param script_name Name for the script (used in error reporting)
     */
    LuaParser(const std::string& script_content, const std::string& script_name);

    /**
     * @brief Destructor
     */
    ~LuaParser() override = default;

    // BaseParser interface implementation
    std::optional<LogEntry> parse(const std::string& raw_message) override;
    bool can_parse(const std::string& raw_message) const override;
    DeviceType get_device_type() const override;
    std::string get_parser_name() const override;
    std::string get_version() const override;
    std::vector<std::string> get_supported_patterns() const override;

    /**
     * @brief Check if the Lua script was loaded successfully
     * 
     * @return true if script is loaded and ready to use, false otherwise
     */
    bool is_valid() const { return lua_engine_ && lua_engine_->is_script_loaded(); }

    /**
     * @brief Get the last error from the Lua engine
     * 
     * @return String describing the last error
     */
    std::string get_last_error() const;

    /**
     * @brief Reload the script (useful for development/testing)
     * 
     * @return true if reload was successful, false otherwise
     */
    bool reload_script();

private:
    std::unique_ptr<LuaEngine> lua_engine_;
    std::string script_path_;
    std::string script_content_;
    std::string script_name_;
    bool is_from_file_;
};

#endif // LIBNETLOG_ENABLE_LUA

} // namespace libnetlog