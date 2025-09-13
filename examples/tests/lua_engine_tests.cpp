#include <gtest/gtest.h>
#include "libnetlog/lua_engine.hpp"
#include "libnetlog/parsers/lua_parser.hpp"
#include <filesystem>
#include <fstream>

class LuaEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test directory
        test_dir_ = std::filesystem::temp_directory_path() / "netlog_lua_tests";
        std::filesystem::create_directories(test_dir_);
    }

    void TearDown() override {
        // Clean up test directory
        if (std::filesystem::exists(test_dir_)) {
            std::filesystem::remove_all(test_dir_);
        }
    }

    std::string create_test_script(const std::string& script_content, const std::string& filename = "test.nlp") {
        auto script_path = test_dir_ / filename;
        std::ofstream file(script_path);
        file << script_content;
        file.close();
        return script_path.string();
    }

    std::filesystem::path test_dir_;
};

// Test basic Lua engine functionality
TEST_F(LuaEngineTest, BasicEngineCreation) {
    libnetlog::LuaEngine engine;
    EXPECT_FALSE(engine.is_script_loaded());
    EXPECT_EQ(engine.get_parser_name(), "");
    EXPECT_EQ(engine.get_device_type(), libnetlog::DeviceType::Unknown);
}

// Test loading a valid script
TEST_F(LuaEngineTest, LoadValidScript) {
    std::string script_content = R"(
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
            entry.timestamp = os.time()
            entry.severity = "info"
            entry.facility = "TEST"
            entry.message = "Test message parsed"
            entry.metadata = {
                test_field = "test_value"
            }
            return entry
        end
    )";

    auto script_path = create_test_script(script_content);
    libnetlog::LuaEngine engine;

    EXPECT_TRUE(engine.load_script(script_path));
    EXPECT_TRUE(engine.is_script_loaded());
    EXPECT_EQ(engine.get_parser_name(), "Test Parser");
    EXPECT_EQ(engine.get_device_type(), libnetlog::DeviceType::CiscoIOS);
}

// Test loading script from string
TEST_F(LuaEngineTest, LoadScriptFromString) {
    std::string script_content = R"(
        function get_parser_name()
            return "String Parser"
        end

        function get_device_type()
            return "Unknown"
        end

        function can_parse(raw_message)
            return true
        end

        function parse(raw_message)
            local entry = netlog.create_log_entry()
            entry.message = raw_message
            return entry
        end
    )";

    libnetlog::LuaEngine engine;
    EXPECT_TRUE(engine.load_script_from_string(script_content, "string_test"));
    EXPECT_TRUE(engine.is_script_loaded());
    EXPECT_EQ(engine.get_parser_name(), "String Parser");
}

// Test parsing functionality
TEST_F(LuaEngineTest, ParseMessage) {
    std::string script_content = R"(
        function get_parser_name()
            return "Parse Test Parser"
        end

        function get_device_type()
            return "GenericSyslog"
        end

        function can_parse(raw_message)
            return string.find(raw_message, "PARSE_TEST") ~= nil
        end

        function parse(raw_message)
            if not can_parse(raw_message) then
                return nil
            end

            local entry = netlog.create_log_entry()
            entry.timestamp = netlog.parse_timestamp("Jan 15 10:30:45")
            entry.severity = "error"
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

    auto script_path = create_test_script(script_content);
    libnetlog::LuaEngine engine;
    ASSERT_TRUE(engine.load_script(script_path));

    // Test can_parse
    EXPECT_TRUE(engine.can_parse("This is a PARSE_TEST message"));
    EXPECT_FALSE(engine.can_parse("This message has no test marker"));

    // Test parse
    auto result = engine.parse("PARSE_TEST: Sample log entry");
    ASSERT_TRUE(result.has_value());

    auto& entry = result.value();
    EXPECT_EQ(entry.get_severity(), libnetlog::Severity::Error);
    EXPECT_EQ(entry.get_facility(), "TEST");
    EXPECT_EQ(entry.get_message(), "Parsed: PARSE_TEST: Sample log entry");
    EXPECT_EQ(entry.get_hostname(), "testhost");
    EXPECT_EQ(entry.get_process_name(), "testprocess");

    auto metadata = entry.get_metadata();
    EXPECT_EQ(metadata.at("original_message"), "PARSE_TEST: Sample log entry");
    EXPECT_EQ(metadata.at("parser_version"), "1.0.0");
}

// Test invalid script handling
TEST_F(LuaEngineTest, LoadInvalidScript) {
    std::string invalid_script = R"(
        -- Missing required functions
        function get_parser_name()
            return "Invalid Parser"
        end

        -- Syntax error
        function invalid_syntax(
            return "broken"
        end
    )";

    auto script_path = create_test_script(invalid_script);
    libnetlog::LuaEngine engine;

    EXPECT_FALSE(engine.load_script(script_path));
    EXPECT_FALSE(engine.is_script_loaded());
    EXPECT_FALSE(engine.get_last_error().empty());
}

// Test missing required functions
TEST_F(LuaEngineTest, MissingRequiredFunctions) {
    std::string incomplete_script = R"(
        function get_parser_name()
            return "Incomplete Parser"
        end

        -- Missing can_parse, parse, get_device_type
    )";

    auto script_path = create_test_script(incomplete_script);
    libnetlog::LuaEngine engine;

    EXPECT_FALSE(engine.load_script(script_path));
    EXPECT_FALSE(engine.is_script_loaded());
}

// Test LuaParser wrapper
TEST_F(LuaEngineTest, LuaParserWrapper) {
    std::string script_content = R"(
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
            return entry
        end

        function get_supported_patterns()
            return {"WRAPPER.*", "TEST.*"}
        end
    )";

    auto script_path = create_test_script(script_content);
    libnetlog::LuaParser parser(script_path);

    EXPECT_TRUE(parser.is_valid());
    EXPECT_EQ(parser.get_parser_name(), "Wrapper Test Parser");
    EXPECT_EQ(parser.get_version(), "2.0.0");
    EXPECT_EQ(parser.get_device_type(), libnetlog::DeviceType::CiscoNXOS);

    auto patterns = parser.get_supported_patterns();
    EXPECT_EQ(patterns.size(), 2);
    EXPECT_EQ(patterns[0], "WRAPPER.*");
    EXPECT_EQ(patterns[1], "TEST.*");

    EXPECT_TRUE(parser.can_parse("WRAPPER test message"));
    EXPECT_FALSE(parser.can_parse("No match here"));

    auto result = parser.parse("WRAPPER test message");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->get_message(), "Wrapped: WRAPPER test message");
    EXPECT_EQ(result->get_severity(), libnetlog::Severity::Warning);
}

// Test parser registry
TEST_F(LuaEngineTest, ParserRegistry) {
    // Create multiple test parsers
    std::string parser1 = R"(
        function get_parser_name() return "Parser1" end
        function get_device_type() return "CiscoIOS" end
        function can_parse(msg) return string.find(msg, "IOS") ~= nil end
        function parse(msg)
            local entry = netlog.create_log_entry()
            entry.message = "IOS: " .. msg
            return entry
        end
    )";

    std::string parser2 = R"(
        function get_parser_name() return "Parser2" end
        function get_device_type() return "CiscoNXOS" end
        function can_parse(msg) return string.find(msg, "NXOS") ~= nil end
        function parse(msg)
            local entry = netlog.create_log_entry()
            entry.message = "NXOS: " .. msg
            return entry
        end
    )";

    auto script1_path = create_test_script(parser1, "parser1.nlp");
    auto script2_path = create_test_script(parser2, "parser2.nlp");

    libnetlog::LuaParserRegistry registry;

    EXPECT_TRUE(registry.register_parser(script1_path));
    EXPECT_TRUE(registry.register_parser(script2_path));
    EXPECT_EQ(registry.size(), 2);

    auto parser_names = registry.list_parsers();
    EXPECT_EQ(parser_names.size(), 2);

    // Test finding parsers by message
    auto* parser_for_ios = registry.find_parser_for_message("This is an IOS message");
    ASSERT_NE(parser_for_ios, nullptr);
    EXPECT_EQ(parser_for_ios->get_parser_name(), "Parser1");

    auto* parser_for_nxos = registry.find_parser_for_message("This is an NXOS message");
    ASSERT_NE(parser_for_nxos, nullptr);
    EXPECT_EQ(parser_for_nxos->get_parser_name(), "Parser2");

    auto* parser_for_unknown = registry.find_parser_for_message("Unknown message type");
    EXPECT_EQ(parser_for_unknown, nullptr);

    // Test getting parser by name
    auto* parser1_by_name = registry.get_parser("Parser1");
    ASSERT_NE(parser1_by_name, nullptr);
    EXPECT_EQ(parser1_by_name->get_parser_name(), "Parser1");

    // Test parser info
    auto info = registry.get_parser_info("Parser1");
    EXPECT_EQ(info["name"], "Parser1");
    EXPECT_EQ(info["device_type"], "CiscoIOS");
}

// Test API functions
TEST_F(LuaEngineTest, APIFunctions) {
    std::string script_content = R"(
        function get_parser_name() return "API Test" end
        function get_device_type() return "Unknown" end
        function can_parse(msg) return true end

        function parse(raw_message)
            -- Test logging functions
            netlog.log_debug("Debug test")
            netlog.log_info("Info test")
            netlog.log_warn("Warning test")
            netlog.log_error("Error test")

            -- Test utility functions
            local entry = netlog.create_log_entry()
            entry.timestamp = netlog.parse_timestamp("Jan 15 10:30:45")
            entry.severity = netlog.parse_severity("error")
            entry.message = "API test message"

            return entry
        end
    )";

    libnetlog::LuaEngine engine;
    ASSERT_TRUE(engine.load_script_from_string(script_content, "api_test"));

    auto result = engine.parse("Test message for API");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->get_message(), "API test message");
    EXPECT_EQ(result->get_severity(), libnetlog::Severity::Error);
}

// Test script validation
TEST_F(LuaEngineTest, ScriptValidation) {
    std::string valid_script = R"(
        function get_parser_name() return "Valid" end
        function get_device_type() return "Unknown" end
        function can_parse(msg) return true end
        function parse(msg)
            local entry = netlog.create_log_entry()
            entry.message = msg
            return entry
        end
    )";

    std::string invalid_script = R"(
        function get_parser_name() return "Invalid" end
        -- Missing required functions and syntax errors
        function broken_function(
    )";

    auto valid_path = create_test_script(valid_script, "valid.nlp");
    auto invalid_path = create_test_script(invalid_script, "invalid.nlp");

    EXPECT_TRUE(libnetlog::LuaEngine::validate_script(valid_path));
    EXPECT_FALSE(libnetlog::LuaEngine::validate_script(invalid_path));
    EXPECT_FALSE(libnetlog::LuaEngine::validate_script("nonexistent.nlp"));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}