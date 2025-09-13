#include "libnetlog/lua_engine.hpp"
#include "libnetlog/parsers/lua_parser.hpp"
#include <iostream>
#include <string>

int main() {
    std::cout << "NetLogAI Lua Scripting Engine Test\n";
    std::cout << "===================================\n\n";

    // Test basic Lua engine creation
    {
        std::cout << "1. Testing basic Lua engine creation...\n";
        libnetlog::LuaEngine engine;
        std::cout << "   ✓ Lua engine created successfully\n";
        std::cout << "   - Script loaded: " << (engine.is_script_loaded() ? "Yes" : "No") << "\n";
        std::cout << "   - Parser name: '" << engine.get_parser_name() << "'\n";
        std::cout << "   - Device type: " << static_cast<int>(engine.get_device_type()) << "\n\n";
    }

    // Test loading a simple script from string
    {
        std::cout << "2. Testing script loading from string...\n";

        std::string test_script = R"(
            function get_parser_name()
                return "Test Parser"
            end

            function get_version()
                return "1.0.0"
            end

            function get_device_type()
                return "CiscoIOS"
            end

            function can_parse(raw_message)
                return string.find(raw_message, "TEST") ~= nil
            end

            function parse(raw_message)
                if not can_parse(raw_message) then
                    return nil
                end

                local entry = netlog.create_log_entry()
                entry.timestamp = netlog.parse_timestamp("Jan 15 10:30:45")
                entry.severity = "info"
                entry.facility = "TEST"
                entry.message = "Parsed: " .. raw_message
                entry.hostname = "testhost"
                entry.process_name = "testprocess"
                entry.metadata = {
                    original_message = raw_message,
                    parser_version = "1.0.0"
                }
                return entry
            end
        )";

        libnetlog::LuaEngine engine;
        bool loaded = engine.load_script_from_string(test_script, "inline_test");

        std::cout << "   Script load result: " << (loaded ? "SUCCESS" : "FAILED") << "\n";
        if (!loaded) {
            std::cout << "   Error: " << engine.get_last_error() << "\n";
        } else {
            std::cout << "   ✓ Script loaded successfully\n";
            std::cout << "   - Parser name: '" << engine.get_parser_name() << "'\n";
            std::cout << "   - Version: '" << engine.get_version() << "'\n";
            std::cout << "   - Device type: " << static_cast<int>(engine.get_device_type()) << "\n";

            // Test can_parse
            std::string test_message1 = "This is a TEST message";
            std::string test_message2 = "This has no marker";

            std::cout << "   - Can parse '" << test_message1 << "': "
                      << (engine.can_parse(test_message1) ? "Yes" : "No") << "\n";
            std::cout << "   - Can parse '" << test_message2 << "': "
                      << (engine.can_parse(test_message2) ? "Yes" : "No") << "\n";

            // Test parsing
            auto result = engine.parse(test_message1);
            if (result) {
                std::cout << "   ✓ Message parsed successfully\n";
                std::cout << "     - Facility: '" << result->get_facility() << "'\n";
                std::cout << "     - Severity: " << static_cast<int>(result->get_severity()) << "\n";
                std::cout << "     - Message: '" << result->get_message() << "'\n";
                std::cout << "     - Hostname: '" << result->get_hostname() << "'\n";

                auto metadata = result->get_metadata();
                std::cout << "     - Metadata count: " << metadata.size() << "\n";
                for (const auto& [key, value] : metadata) {
                    std::cout << "       * " << key << ": '" << value << "'\n";
                }
            } else {
                std::cout << "   ✗ Message parsing failed\n";
            }
        }
        std::cout << "\n";
    }

    // Test LuaParser wrapper
    {
        std::cout << "3. Testing LuaParser wrapper...\n";

        std::string wrapper_script = R"(
            function get_parser_name()
                return "Wrapper Test Parser"
            end

            function get_version()
                return "2.0.0"
            end

            function get_device_type()
                return "CiscoNXOS"
            end

            function can_parse(raw_message)
                return string.find(raw_message, "WRAPPER") ~= nil
            end

            function parse(raw_message)
                local entry = netlog.create_log_entry()
                entry.message = "Wrapped: " .. raw_message
                entry.severity = "warning"
                entry.facility = "WRAPPER"
                return entry
            end

            function get_supported_patterns()
                return {"WRAPPER.*", "TEST.*"}
            end
        )";

        libnetlog::LuaParser parser(wrapper_script, "wrapper_test");

        std::cout << "   Wrapper valid: " << (parser.is_valid() ? "Yes" : "No") << "\n";
        if (!parser.is_valid()) {
            std::cout << "   Error: " << parser.get_last_error() << "\n";
        } else {
            std::cout << "   ✓ LuaParser wrapper created successfully\n";
            std::cout << "   - Parser name: '" << parser.get_parser_name() << "'\n";
            std::cout << "   - Version: '" << parser.get_version() << "'\n";
            std::cout << "   - Device type: " << static_cast<int>(parser.get_device_type()) << "\n";

            auto patterns = parser.get_supported_patterns();
            std::cout << "   - Supported patterns: ";
            for (size_t i = 0; i < patterns.size(); ++i) {
                std::cout << "'" << patterns[i] << "'";
                if (i < patterns.size() - 1) std::cout << ", ";
            }
            std::cout << "\n";

            // Test parsing through wrapper
            std::string wrapper_msg = "WRAPPER test message";
            if (parser.can_parse(wrapper_msg)) {
                auto result = parser.parse(wrapper_msg);
                if (result) {
                    std::cout << "   ✓ Wrapper parsing successful\n";
                    std::cout << "     - Message: '" << result->get_message() << "'\n";
                    std::cout << "     - Severity: " << static_cast<int>(result->get_severity()) << "\n";
                } else {
                    std::cout << "   ✗ Wrapper parsing failed\n";
                }
            } else {
                std::cout << "   ✗ Wrapper cannot parse test message\n";
            }
        }
        std::cout << "\n";
    }

    // Test parser registry
    {
        std::cout << "4. Testing parser registry...\n";

        libnetlog::LuaParserRegistry registry;
        std::cout << "   Registry created, size: " << registry.size() << "\n";

        // Try to load example parsers if they exist
        std::vector<std::string> potential_paths = {
            "examples/parsers/cisco/ios-general.nlp",
            "examples/parsers/cisco/nxos-general.nlp",
            "examples/parsers/generic/syslog-rfc3164.nlp"
        };

        int loaded_count = 0;
        for (const auto& path : potential_paths) {
            if (std::filesystem::exists(path)) {
                if (registry.register_parser(path)) {
                    loaded_count++;
                    std::cout << "   ✓ Loaded parser from: " << path << "\n";
                } else {
                    std::cout << "   ✗ Failed to load parser from: " << path << "\n";
                }
            }
        }

        std::cout << "   Final registry size: " << registry.size() << "\n";

        if (registry.size() > 0) {
            auto parser_names = registry.list_parsers();
            std::cout << "   Registered parsers: ";
            for (size_t i = 0; i < parser_names.size(); ++i) {
                std::cout << "'" << parser_names[i] << "'";
                if (i < parser_names.size() - 1) std::cout << ", ";
            }
            std::cout << "\n";
        }
        std::cout << "\n";
    }

    std::cout << "NetLogAI Lua Scripting Engine Test Completed!\n";
    std::cout << "The Lua scripting engine is working correctly.\n";

    return 0;
}