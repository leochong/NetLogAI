#include <gtest/gtest.h>
#include "libnetlog/parsers/lua_parser.hpp"
#include "libnetlog/lua_engine.hpp"
#include <filesystem>
#include <fstream>

class SampleLogTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Load our example parsers
        ios_parser_path_ = find_parser_script("ios-general.nlp");
        nxos_parser_path_ = find_parser_script("nxos-general.nlp");
        asa_parser_path_ = find_parser_script("asa-firewall.nlp");
        syslog_parser_path_ = find_parser_script("syslog-rfc3164.nlp");
    }

    std::string find_parser_script(const std::string& filename) {
        // Look in various possible locations
        std::vector<std::filesystem::path> search_paths = {
            std::filesystem::current_path() / "examples" / "parsers" / "cisco" / filename,
            std::filesystem::current_path() / "examples" / "parsers" / "generic" / filename,
            std::filesystem::current_path() / ".." / "examples" / "parsers" / "cisco" / filename,
            std::filesystem::current_path() / ".." / "examples" / "parsers" / "generic" / filename
        };

        for (const auto& path : search_paths) {
            if (std::filesystem::exists(path)) {
                return path.string();
            }
        }

        // Try to find the file in the project structure
        auto current = std::filesystem::current_path();
        while (current.has_parent_path() && current != current.parent_path()) {
            auto test_path = current / "examples" / "parsers";
            if (std::filesystem::exists(test_path)) {
                for (auto& entry : std::filesystem::recursive_directory_iterator(test_path)) {
                    if (entry.is_regular_file() && entry.path().filename() == filename) {
                        return entry.path().string();
                    }
                }
            }
            current = current.parent_path();
        }

        return ""; // Not found
    }

    std::string ios_parser_path_;
    std::string nxos_parser_path_;
    std::string asa_parser_path_;
    std::string syslog_parser_path_;
};

// Test Cisco IOS parser with real-world samples
TEST_F(SampleLogTest, CiscoIOSRealWorldSamples) {
    if (ios_parser_path_.empty()) {
        GTEST_SKIP() << "IOS parser script not found";
    }

    libnetlog::LuaParser parser(ios_parser_path_);
    ASSERT_TRUE(parser.is_valid()) << "Failed to load IOS parser: " << parser.get_last_error();

    struct TestCase {
        std::string message;
        bool should_parse;
        std::string expected_facility;
        std::string expected_event_type;
    };

    std::vector<TestCase> test_cases = {
        {
            "%LINEPROTO-5-UPDOWN: Line protocol on Interface GigabitEthernet0/1, changed state to down",
            true, "LINEPROTO", "interface_state_change"
        },
        {
            "123: Jan 15 10:30:45: %BGP-3-NOTIFICATION: sent to neighbor 192.168.1.2 4/0 (hold time expired)",
            true, "BGP", "bgp_notification"
        },
        {
            "%SYS-5-CONFIG_I: Configured from console by admin on vty0 (192.168.1.100)",
            true, "SYS", "configuration_change"
        },
        {
            "%OSPF-5-ADJCHG: Process 1, Nbr 10.0.0.2 on FastEthernet0/0 from FULL to DOWN",
            true, "OSPF", "ospf_adjacency_change"
        },
        {
            "This is not a Cisco IOS message",
            false, "", ""
        }
    };

    for (size_t i = 0; i < test_cases.size(); ++i) {
        const auto& test_case = test_cases[i];

        bool can_parse = parser.can_parse(test_case.message);
        EXPECT_EQ(can_parse, test_case.should_parse)
            << "Test case " << i << " can_parse mismatch for: " << test_case.message;

        if (test_case.should_parse) {
            auto result = parser.parse(test_case.message);
            ASSERT_TRUE(result.has_value())
                << "Test case " << i << " failed to parse: " << test_case.message;

            EXPECT_EQ(result->get_facility(), test_case.expected_facility)
                << "Test case " << i << " facility mismatch";

            auto metadata = result->get_metadata();
            if (!test_case.expected_event_type.empty()) {
                EXPECT_EQ(metadata.at("event_type"), test_case.expected_event_type)
                    << "Test case " << i << " event_type mismatch";
            }
        }
    }
}

// Test Cisco NX-OS parser with real-world samples
TEST_F(SampleLogTest, CiscoNXOSRealWorldSamples) {
    if (nxos_parser_path_.empty()) {
        GTEST_SKIP() << "NX-OS parser script not found";
    }

    libnetlog::LuaParser parser(nxos_parser_path_);
    ASSERT_TRUE(parser.is_valid()) << "Failed to load NX-OS parser: " << parser.get_last_error();

    std::vector<std::string> test_messages = {
        "2024 Jan 15 10:30:45 nxos-switch01 %ETHPORT-5-IF_DOWN_ADMIN_DOWN: Interface Ethernet1/1 is down (Administratively down)",
        "2024 Jan 15 10:31:02 nxos-switch01 %VSHD-5-VSHD_SYSLOG_CONFIG_I: Configured from vty by admin on 192.168.1.100",
        "2024 Jan 15 10:31:15 nxos-switch01 %$ VDC-1 %$ %SYSMGR-2-SERVICE_CRASHED: Service \"ospf\" crashed",
        "2024 Jan 15 10:32:00 nxos-switch01 %PORT_CHANNEL-5-IF_UP: Interface port-channel10 is up"
    };

    for (const auto& message : test_messages) {
        EXPECT_TRUE(parser.can_parse(message))
            << "NX-OS parser should be able to parse: " << message;

        auto result = parser.parse(message);
        ASSERT_TRUE(result.has_value())
            << "NX-OS parser failed to parse: " << message;

        EXPECT_EQ(result->get_device_type(), libnetlog::DeviceType::CiscoNXOS);
        EXPECT_FALSE(result->get_message().empty());
    }
}

// Test Cisco ASA parser with firewall samples
TEST_F(SampleLogTest, CiscoASAFirewallSamples) {
    if (asa_parser_path_.empty()) {
        GTEST_SKIP() << "ASA parser script not found";
    }

    libnetlog::LuaParser parser(asa_parser_path_);
    ASSERT_TRUE(parser.is_valid()) << "Failed to load ASA parser: " << parser.get_last_error();

    struct ASATestCase {
        std::string message;
        std::string expected_event_type;
        std::string expected_action;
    };

    std::vector<ASATestCase> test_cases = {
        {
            "%ASA-6-302013: Built inbound TCP connection 12345 for outside:192.168.1.100/443 to inside:10.0.0.50/8080",
            "connection_built", ""
        },
        {
            "%ASA-4-106023: Deny tcp src outside:192.168.1.200/12345 dst inside:10.0.0.100/80 by access-group \"outside_access_in\"",
            "access_control_decision", "deny"
        },
        {
            "%ASA-6-725001: Starting SSL handshake with client outside:192.168.1.150/443 for TLSv1.2 session",
            "ssl_vpn_handshake_start", ""
        }
    };

    for (const auto& test_case : test_cases) {
        EXPECT_TRUE(parser.can_parse(test_case.message))
            << "ASA parser should be able to parse: " << test_case.message;

        auto result = parser.parse(test_case.message);
        ASSERT_TRUE(result.has_value())
            << "ASA parser failed to parse: " << test_case.message;

        EXPECT_EQ(result->get_device_type(), libnetlog::DeviceType::CiscoASA);

        auto metadata = result->get_metadata();
        if (!test_case.expected_event_type.empty()) {
            EXPECT_EQ(metadata.at("event_type"), test_case.expected_event_type);
        }
        if (!test_case.expected_action.empty()) {
            EXPECT_EQ(metadata.at("action"), test_case.expected_action);
        }
    }
}

// Test generic syslog parser
TEST_F(SampleLogTest, GenericSyslogSamples) {
    if (syslog_parser_path_.empty()) {
        GTEST_SKIP() << "Syslog parser script not found";
    }

    libnetlog::LuaParser parser(syslog_parser_path_);
    ASSERT_TRUE(parser.is_valid()) << "Failed to load Syslog parser: " << parser.get_last_error();

    std::vector<std::string> test_messages = {
        "<34>Jan 15 10:30:45 server01 sshd[1234]: Accepted password for admin from 192.168.1.100",
        "<165>Jan 15 10:31:02 router01 kernel: eth0: link up",
        "<86>Jan 15 10:31:15 switch01 snmpd[5678]: Connection from UDP: [192.168.1.200]:161",
        "<38>Jan 15 10:32:00 server01 CRON[9999]: (root) CMD (/usr/bin/system-backup.sh)"
    };

    for (const auto& message : test_messages) {
        EXPECT_TRUE(parser.can_parse(message))
            << "Syslog parser should be able to parse: " << message;

        auto result = parser.parse(message);
        ASSERT_TRUE(result.has_value())
            << "Syslog parser failed to parse: " << message;

        EXPECT_EQ(result->get_device_type(), libnetlog::DeviceType::GenericSyslog);
        EXPECT_FALSE(result->get_hostname().empty());
    }
}

// Test parser selection with registry
TEST_F(SampleLogTest, AutomaticParserSelection) {
    libnetlog::LuaParserRegistry registry;

    int loaded_count = 0;
    if (!ios_parser_path_.empty()) {
        EXPECT_TRUE(registry.register_parser(ios_parser_path_));
        loaded_count++;
    }
    if (!nxos_parser_path_.empty()) {
        EXPECT_TRUE(registry.register_parser(nxos_parser_path_));
        loaded_count++;
    }
    if (!asa_parser_path_.empty()) {
        EXPECT_TRUE(registry.register_parser(asa_parser_path_));
        loaded_count++;
    }
    if (!syslog_parser_path_.empty()) {
        EXPECT_TRUE(registry.register_parser(syslog_parser_path_));
        loaded_count++;
    }

    if (loaded_count == 0) {
        GTEST_SKIP() << "No parser scripts found for testing";
    }

    struct SelectionTestCase {
        std::string message;
        std::string expected_parser_type;
    };

    std::vector<SelectionTestCase> test_cases = {
        {"%LINEPROTO-5-UPDOWN: Line protocol down", "ios"},
        {"2024 Jan 15 10:30:45 nxos01 %ETHPORT-5-IF_DOWN: Interface down", "nxos"},
        {"%ASA-6-302013: Built connection", "asa"},
        {"<34>Jan 15 10:30:45 server01 sshd[1234]: Login", "syslog"}
    };

    for (const auto& test_case : test_cases) {
        auto* parser = registry.find_parser_for_message(test_case.message);
        if (parser) {
            auto result = parser->parse(test_case.message);
            EXPECT_TRUE(result.has_value())
                << "Auto-selected parser failed to parse: " << test_case.message;
        }
    }
}

// Stress test with many messages
TEST_F(SampleLogTest, StressTestParsing) {
    if (ios_parser_path_.empty()) {
        GTEST_SKIP() << "IOS parser script not found for stress test";
    }

    libnetlog::LuaParser parser(ios_parser_path_);
    ASSERT_TRUE(parser.is_valid());

    const std::string base_message = "%LINEPROTO-5-UPDOWN: Line protocol on Interface GigabitEthernet0/";

    auto start_time = std::chrono::high_resolution_clock::now();

    const int num_messages = 1000;
    int successful_parses = 0;

    for (int i = 0; i < num_messages; ++i) {
        std::string message = base_message + std::to_string(i) + ", changed state to down";

        if (parser.can_parse(message)) {
            auto result = parser.parse(message);
            if (result.has_value()) {
                successful_parses++;
            }
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    EXPECT_EQ(successful_parses, num_messages);

    // Should be able to parse 1000 messages in reasonable time (under 1 second)
    EXPECT_LT(duration.count(), 1000)
        << "Parsing " << num_messages << " messages took " << duration.count() << "ms";

    std::cout << "Parsed " << successful_parses << " messages in " << duration.count() << "ms\n";
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}