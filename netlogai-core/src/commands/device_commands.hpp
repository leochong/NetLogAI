#pragma once

#include "../cli/command_line.hpp"
#include <string>
#include <vector>
#include <memory>

namespace netlogai::commands {

// Device connection types
enum class DeviceConnectionType {
    SSH,
    SNMP,
    HTTP,
    TELNET
};

// Device authentication methods
enum class DeviceAuthType {
    PASSWORD,
    SSH_KEY,
    SNMP_COMMUNITY,
    TOKEN
};

// Device profile structure
struct DeviceProfile {
    std::string id;
    std::string name;
    std::string hostname;
    int port;
    DeviceConnectionType connection_type;
    DeviceAuthType auth_type;
    std::string username;
    std::string password;           // Encrypted storage
    std::string ssh_key_path;
    std::string snmp_community;
    std::string device_type;        // cisco-ios, cisco-nxos, etc.
    std::string log_path;           // Remote log file path
    int timeout_seconds;
    bool enabled;
    std::vector<std::string> commands; // Custom commands for log collection
};

class DeviceCommands {
public:
    static void register_commands(cli::CommandLine& cli);

private:
    // Device management commands
    static int add_device(const cli::CommandArgs& args);
    static int remove_device(const cli::CommandArgs& args);
    static int list_devices(const cli::CommandArgs& args);
    static int show_device(const cli::CommandArgs& args);
    static int edit_device(const cli::CommandArgs& args);
    static int test_device(const cli::CommandArgs& args);

    // Device connection commands
    static int connect_device(const cli::CommandArgs& args);
    static int fetch_logs(const cli::CommandArgs& args);
    static int fetch_all(const cli::CommandArgs& args);

    // Device discovery commands
    static int discover_devices(const cli::CommandArgs& args);
    static int scan_network(const cli::CommandArgs& args);

    // Helper functions
    static void show_device_help();
    static std::string get_device_config_path();
    static bool load_device_profiles();
    static bool save_device_profiles();
    static DeviceProfile* find_device_by_id(const std::string& id);
    static DeviceProfile* find_device_by_name(const std::string& name);
    static bool validate_device_profile(const DeviceProfile& profile);
    static std::string encrypt_password(const std::string& password);
    static std::string decrypt_password(const std::string& encrypted);
    static bool test_ssh_connection(const DeviceProfile& profile);
    static bool test_snmp_connection(const DeviceProfile& profile);
    static std::vector<std::string> execute_remote_command(const DeviceProfile& profile, const std::string& command);
    static std::vector<std::string> collect_device_logs(const DeviceProfile& profile);

    // Network discovery helpers
    static std::vector<std::string> scan_ip_range(const std::string& cidr);
    static std::string detect_device_type(const std::string& hostname, int port);
    static bool is_network_device(const std::string& hostname, int port);

    // Storage for device profiles
    static std::vector<DeviceProfile> device_profiles;
};

} // namespace netlogai::commands