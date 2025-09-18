#include "device_commands.hpp"
#include "networking/telnet_client.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace netlogai::commands {

// Static member initialization
std::vector<DeviceProfile> DeviceCommands::device_profiles;

void DeviceCommands::register_commands(cli::CommandLine& cli) {
    // Load existing device profiles
    load_device_profiles();

    // Register device management commands
    cli.register_subcommand("device", "add", add_device, "Add a new network device");
    cli.register_subcommand("device", "remove", remove_device, "Remove a network device");
    cli.register_subcommand("device", "list", list_devices, "List all configured devices");
    cli.register_subcommand("device", "show", show_device, "Show device details");
    cli.register_subcommand("device", "edit", edit_device, "Edit device configuration");
    cli.register_subcommand("device", "test", test_device, "Test device connectivity");

    // Register connection and fetching commands
    cli.register_subcommand("device", "connect", connect_device, "Connect to a device interactively");
    cli.register_command("fetch", fetch_logs, "Fetch logs from network devices");
    cli.register_subcommand("fetch", "all", fetch_all, "Fetch logs from all devices");

    // Register discovery commands
    cli.register_subcommand("device", "discover", discover_devices, "Auto-discover network devices");
    cli.register_subcommand("device", "scan", scan_network, "Scan network range for devices");

    // Register GNS3 integration commands
    cli.register_subcommand("device", "add-gns3", add_gns3_device, "Add GNS3 device with console access");
    cli.register_subcommand("device", "discover-gns3", discover_gns3_lab, "Discover devices in GNS3 lab");
    cli.register_subcommand("fetch", "gns3", fetch_gns3_logs, "Fetch logs from GNS3 devices");

    // Register help
    cli.register_subcommand("device", "help", [](const cli::CommandArgs&) {
        show_device_help();
        return 0;
    }, "Show device management help");
}

int DeviceCommands::add_device(const cli::CommandArgs& args) {
    if (args.arg_count() < 1) {
        std::cout << "Usage: netlogai device add <hostname> [options]\n";
        std::cout << "Options:\n";
        std::cout << "  --name <name>         Device friendly name\n";
        std::cout << "  --type <type>         Device type (cisco-ios, cisco-nxos, cisco-asa, generic)\n";
        std::cout << "  --port <port>         Connection port (default: 22 for SSH)\n";
        std::cout << "  --username <user>     Username for authentication\n";
        std::cout << "  --password            Prompt for password\n";
        std::cout << "  --key <path>          SSH private key path\n";
        std::cout << "  --connection <type>   Connection type (ssh, snmp, http, telnet)\n";
        std::cout << "  --timeout <seconds>   Connection timeout (default: 30)\n";
        return 1;
    }

    DeviceProfile profile;
    profile.id = std::to_string(std::hash<std::string>{}(args.get_arg(0) + std::to_string(time(nullptr))));
    profile.hostname = args.get_arg(0);
    profile.name = args.get_option("name", profile.hostname);
    profile.device_type = args.get_option("type", "cisco-ios");
    profile.port = std::stoi(args.get_option("port", "22"));
    profile.username = args.get_option("username", "admin");
    profile.connection_type = DeviceConnectionType::SSH;
    profile.auth_type = DeviceAuthType::PASSWORD;
    profile.timeout_seconds = std::stoi(args.get_option("timeout", "30"));
    profile.enabled = true;

    // Handle connection type
    std::string conn_type = args.get_option("connection", "ssh");
    if (conn_type == "ssh") {
        profile.connection_type = DeviceConnectionType::SSH;
        if (profile.port == 0) profile.port = 22;
    } else if (conn_type == "snmp") {
        profile.connection_type = DeviceConnectionType::SNMP;
        profile.auth_type = DeviceAuthType::SNMP_COMMUNITY;
        if (profile.port == 0) profile.port = 161;
    } else if (conn_type == "telnet") {
        profile.connection_type = DeviceConnectionType::TELNET;
        if (profile.port == 0) profile.port = 23;
    }

    // Handle authentication
    if (args.has_flag("password")) {
        std::cout << "Enter password for " << profile.username << "@" << profile.hostname << ": ";
        std::string password;
        std::getline(std::cin, password);
        profile.password = encrypt_password(password);
    }

    std::string ssh_key = args.get_option("key");
    if (!ssh_key.empty()) {
        profile.ssh_key_path = ssh_key;
        profile.auth_type = DeviceAuthType::SSH_KEY;
    }

    // Set default log collection commands based on device type
    if (profile.device_type == "cisco-ios" || profile.device_type == "cisco-nxos") {
        profile.commands = {"show logging", "show logging last 100"};
    } else if (profile.device_type == "cisco-asa") {
        profile.commands = {"show logging", "show logging buffer"};
    }

    // Validate profile
    if (!validate_device_profile(profile)) {
        std::cout << "Error: Invalid device profile configuration.\n";
        return 1;
    }

    // Add to profiles
    device_profiles.push_back(profile);

    // Save to file
    if (!save_device_profiles()) {
        std::cout << "Warning: Failed to save device profiles to file.\n";
    }

    std::cout << "Device added successfully:\n";
    std::cout << "  ID: " << profile.id << "\n";
    std::cout << "  Name: " << profile.name << "\n";
    std::cout << "  Hostname: " << profile.hostname << ":" << profile.port << "\n";
    std::cout << "  Type: " << profile.device_type << "\n";

    return 0;
}

int DeviceCommands::remove_device(const cli::CommandArgs& args) {
    if (args.arg_count() < 1) {
        std::cout << "Usage: netlogai device remove <device-id|name>\n";
        return 1;
    }

    std::string identifier = args.get_arg(0);
    auto it = std::find_if(device_profiles.begin(), device_profiles.end(),
        [&identifier](const DeviceProfile& profile) {
            return profile.id == identifier || profile.name == identifier;
        });

    if (it == device_profiles.end()) {
        std::cout << "Error: Device not found: " << identifier << "\n";
        return 1;
    }

    std::cout << "Removing device: " << it->name << " (" << it->hostname << ")\n";
    device_profiles.erase(it);

    if (!save_device_profiles()) {
        std::cout << "Warning: Failed to save device profiles to file.\n";
    }

    std::cout << "Device removed successfully.\n";
    return 0;
}

int DeviceCommands::list_devices(const cli::CommandArgs& args) {
    if (device_profiles.empty()) {
        std::cout << "No devices configured.\n";
        std::cout << "Use 'netlogai device add <hostname>' to add a device.\n";
        return 0;
    }

    std::cout << "Configured Network Devices:\n";
    std::cout << "===========================\n\n";

    // Table header
    std::cout << std::left << std::setw(20) << "Name"
              << std::setw(25) << "Hostname:Port"
              << std::setw(15) << "Type"
              << std::setw(10) << "Connection"
              << std::setw(10) << "Status\n";
    std::cout << std::string(80, '-') << "\n";

    for (const auto& profile : device_profiles) {
        std::string connection_str;
        switch (profile.connection_type) {
            case DeviceConnectionType::SSH: connection_str = "SSH"; break;
            case DeviceConnectionType::SNMP: connection_str = "SNMP"; break;
            case DeviceConnectionType::HTTP: connection_str = "HTTP"; break;
            case DeviceConnectionType::TELNET: connection_str = "TELNET"; break;
        }

        std::string status = profile.enabled ? "Enabled" : "Disabled";
        std::string endpoint = profile.hostname + ":" + std::to_string(profile.port);

        std::cout << std::left << std::setw(20) << profile.name
                  << std::setw(25) << endpoint
                  << std::setw(15) << profile.device_type
                  << std::setw(10) << connection_str
                  << std::setw(10) << status << "\n";
    }

    std::cout << "\nUse 'netlogai device show <name>' for detailed information.\n";
    return 0;
}

int DeviceCommands::show_device(const cli::CommandArgs& args) {
    if (args.arg_count() < 1) {
        std::cout << "Usage: netlogai device show <device-id|name>\n";
        return 1;
    }

    std::string identifier = args.get_arg(0);
    DeviceProfile* profile = find_device_by_id(identifier);
    if (!profile) {
        profile = find_device_by_name(identifier);
    }

    if (!profile) {
        std::cout << "Error: Device not found: " << identifier << "\n";
        return 1;
    }

    std::cout << "Device Details: " << profile->name << "\n";
    std::cout << "==============================\n";
    std::cout << "ID: " << profile->id << "\n";
    std::cout << "Hostname: " << profile->hostname << "\n";
    std::cout << "Port: " << profile->port << "\n";
    std::cout << "Device Type: " << profile->device_type << "\n";

    std::cout << "Connection Type: ";
    switch (profile->connection_type) {
        case DeviceConnectionType::SSH: std::cout << "SSH\n"; break;
        case DeviceConnectionType::SNMP: std::cout << "SNMP\n"; break;
        case DeviceConnectionType::HTTP: std::cout << "HTTP\n"; break;
        case DeviceConnectionType::TELNET: std::cout << "TELNET\n"; break;
    }

    std::cout << "Authentication: ";
    switch (profile->auth_type) {
        case DeviceAuthType::PASSWORD: std::cout << "Username/Password\n"; break;
        case DeviceAuthType::SSH_KEY: std::cout << "SSH Key (" << profile->ssh_key_path << ")\n"; break;
        case DeviceAuthType::SNMP_COMMUNITY: std::cout << "SNMP Community\n"; break;
        case DeviceAuthType::TOKEN: std::cout << "Token\n"; break;
    }

    std::cout << "Username: " << profile->username << "\n";
    std::cout << "Timeout: " << profile->timeout_seconds << " seconds\n";
    std::cout << "Status: " << (profile->enabled ? "Enabled" : "Disabled") << "\n";

    if (!profile->commands.empty()) {
        std::cout << "Log Collection Commands:\n";
        for (const auto& cmd : profile->commands) {
            std::cout << "  - " << cmd << "\n";
        }
    }

    return 0;
}

int DeviceCommands::test_device(const cli::CommandArgs& args) {
    if (args.arg_count() < 1) {
        std::cout << "Usage: netlogai device test <device-id|name>\n";
        return 1;
    }

    std::string identifier = args.get_arg(0);
    DeviceProfile* profile = find_device_by_id(identifier);
    if (!profile) {
        profile = find_device_by_name(identifier);
    }

    if (!profile) {
        std::cout << "Error: Device not found: " << identifier << "\n";
        return 1;
    }

    std::cout << "Testing connection to " << profile->name << " (" << profile->hostname << ":" << profile->port << ")...\n";

    bool success = false;
    switch (profile->connection_type) {
        case DeviceConnectionType::SSH:
            success = test_ssh_connection(*profile);
            break;
        case DeviceConnectionType::SNMP:
            success = test_snmp_connection(*profile);
            break;
        default:
            std::cout << "Connection type not yet implemented for testing.\n";
            return 1;
    }

    if (success) {
        std::cout << "✓ Connection test successful!\n";
        return 0;
    } else {
        std::cout << "✗ Connection test failed.\n";
        return 1;
    }
}

int DeviceCommands::fetch_logs(const cli::CommandArgs& args) {
    if (args.arg_count() < 1 && !args.has_flag("all")) {
        std::cout << "Usage: netlogai fetch <device-id|name> [options]\n";
        std::cout << "       netlogai fetch --all\n";
        std::cout << "Options:\n";
        std::cout << "  --output <file>       Save logs to file\n";
        std::cout << "  --format <format>     Output format (json, text, csv)\n";
        std::cout << "  --lines <count>       Number of recent lines to fetch\n";
        return 1;
    }

    if (args.has_flag("all")) {
        return fetch_all(args);
    }

    std::string identifier = args.get_arg(0);
    DeviceProfile* profile = find_device_by_id(identifier);
    if (!profile) {
        profile = find_device_by_name(identifier);
    }

    if (!profile) {
        std::cout << "Error: Device not found: " << identifier << "\n";
        return 1;
    }

    if (!profile->enabled) {
        std::cout << "Error: Device is disabled: " << profile->name << "\n";
        return 1;
    }

    std::cout << "Fetching logs from " << profile->name << " (" << profile->hostname << ")...\n";

    auto logs = collect_device_logs(*profile);
    if (logs.empty()) {
        std::cout << "No logs collected from device.\n";
        return 1;
    }

    std::string output_file = args.get_option("output", "");
    std::string format = args.get_option("format", "text");

    if (!output_file.empty()) {
        std::ofstream outfile(output_file);
        if (!outfile) {
            std::cout << "Error: Unable to write to file: " << output_file << "\n";
            return 1;
        }

        for (const auto& line : logs) {
            outfile << line << "\n";
        }

        std::cout << "Logs saved to: " << output_file << "\n";
        std::cout << "Lines collected: " << logs.size() << "\n";
    } else {
        std::cout << "Collected Logs:\n";
        std::cout << "===============\n";
        for (const auto& line : logs) {
            std::cout << line << "\n";
        }
    }

    return 0;
}

int DeviceCommands::fetch_all(const cli::CommandArgs& args) {
    if (device_profiles.empty()) {
        std::cout << "No devices configured.\n";
        return 1;
    }

    int success_count = 0;
    int total_count = 0;

    for (const auto& profile : device_profiles) {
        if (!profile.enabled) {
            continue;
        }

        total_count++;
        std::cout << "Fetching logs from " << profile.name << "...\n";

        auto logs = collect_device_logs(profile);
        if (!logs.empty()) {
            success_count++;

            // Save to device-specific file
            std::string filename = profile.name + "_logs.txt";
            std::ofstream outfile(filename);
            for (const auto& line : logs) {
                outfile << line << "\n";
            }

            std::cout << "  ✓ Collected " << logs.size() << " lines -> " << filename << "\n";
        } else {
            std::cout << "  ✗ Failed to collect logs\n";
        }
    }

    std::cout << "\nSummary: " << success_count << "/" << total_count << " devices successful\n";
    return (success_count == total_count) ? 0 : 1;
}

// Helper function implementations
std::string DeviceCommands::get_device_config_path() {
    // Use same config directory as other NetLogAI configs
    char* home = getenv("USERPROFILE");  // Windows
    if (!home) home = getenv("HOME");    // Unix

    std::string config_dir = home ? std::string(home) : ".";
    config_dir += "/.netlogai";

    return config_dir + "/devices.json";
}

bool DeviceCommands::load_device_profiles() {
    std::string config_path = get_device_config_path();
    std::ifstream file(config_path);

    if (!file) {
        // File doesn't exist yet, start with empty profiles
        device_profiles.clear();
        return true;
    }

    try {
        json j;
        file >> j;

        device_profiles.clear();
        for (const auto& item : j["devices"]) {
            DeviceProfile profile;
            profile.id = item["id"];
            profile.name = item["name"];
            profile.hostname = item["hostname"];
            profile.port = item["port"];
            profile.device_type = item["device_type"];
            profile.username = item["username"];
            profile.password = item.value("password", "");
            profile.ssh_key_path = item.value("ssh_key_path", "");
            profile.timeout_seconds = item.value("timeout_seconds", 30);
            profile.enabled = item.value("enabled", true);

            // Parse connection type
            std::string conn_type = item.value("connection_type", "ssh");
            if (conn_type == "ssh") profile.connection_type = DeviceConnectionType::SSH;
            else if (conn_type == "snmp") profile.connection_type = DeviceConnectionType::SNMP;
            else if (conn_type == "http") profile.connection_type = DeviceConnectionType::HTTP;
            else if (conn_type == "telnet") profile.connection_type = DeviceConnectionType::TELNET;

            // Parse auth type
            std::string auth_type = item.value("auth_type", "password");
            if (auth_type == "password") profile.auth_type = DeviceAuthType::PASSWORD;
            else if (auth_type == "ssh_key") profile.auth_type = DeviceAuthType::SSH_KEY;
            else if (auth_type == "snmp_community") profile.auth_type = DeviceAuthType::SNMP_COMMUNITY;
            else if (auth_type == "token") profile.auth_type = DeviceAuthType::TOKEN;

            // Load commands
            if (item.contains("commands")) {
                for (const auto& cmd : item["commands"]) {
                    profile.commands.push_back(cmd);
                }
            }

            device_profiles.push_back(profile);
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading device profiles: " << e.what() << std::endl;
        return false;
    }
}

bool DeviceCommands::save_device_profiles() {
    std::string config_path = get_device_config_path();

    // Ensure config directory exists
    std::string config_dir = config_path.substr(0, config_path.find_last_of("/\\"));
    // Note: For production, should use proper directory creation

    try {
        json j;
        j["devices"] = json::array();

        for (const auto& profile : device_profiles) {
            json device;
            device["id"] = profile.id;
            device["name"] = profile.name;
            device["hostname"] = profile.hostname;
            device["port"] = profile.port;
            device["device_type"] = profile.device_type;
            device["username"] = profile.username;
            device["password"] = profile.password;
            device["ssh_key_path"] = profile.ssh_key_path;
            device["timeout_seconds"] = profile.timeout_seconds;
            device["enabled"] = profile.enabled;

            // Save connection type
            switch (profile.connection_type) {
                case DeviceConnectionType::SSH: device["connection_type"] = "ssh"; break;
                case DeviceConnectionType::SNMP: device["connection_type"] = "snmp"; break;
                case DeviceConnectionType::HTTP: device["connection_type"] = "http"; break;
                case DeviceConnectionType::TELNET: device["connection_type"] = "telnet"; break;
            }

            // Save auth type
            switch (profile.auth_type) {
                case DeviceAuthType::PASSWORD: device["auth_type"] = "password"; break;
                case DeviceAuthType::SSH_KEY: device["auth_type"] = "ssh_key"; break;
                case DeviceAuthType::SNMP_COMMUNITY: device["auth_type"] = "snmp_community"; break;
                case DeviceAuthType::TOKEN: device["auth_type"] = "token"; break;
            }

            // Save commands
            device["commands"] = json::array();
            for (const auto& cmd : profile.commands) {
                device["commands"].push_back(cmd);
            }

            j["devices"].push_back(device);
        }

        std::ofstream file(config_path);
        file << j.dump(2);

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving device profiles: " << e.what() << std::endl;
        return false;
    }
}

DeviceProfile* DeviceCommands::find_device_by_id(const std::string& id) {
    auto it = std::find_if(device_profiles.begin(), device_profiles.end(),
        [&id](const DeviceProfile& profile) {
            return profile.id == id;
        });
    return (it != device_profiles.end()) ? &(*it) : nullptr;
}

DeviceProfile* DeviceCommands::find_device_by_name(const std::string& name) {
    auto it = std::find_if(device_profiles.begin(), device_profiles.end(),
        [&name](const DeviceProfile& profile) {
            return profile.name == name;
        });
    return (it != device_profiles.end()) ? &(*it) : nullptr;
}

bool DeviceCommands::validate_device_profile(const DeviceProfile& profile) {
    if (profile.hostname.empty()) return false;
    if (profile.name.empty()) return false;
    if (profile.port <= 0 || profile.port > 65535) return false;
    if (profile.timeout_seconds <= 0) return false;
    return true;
}

std::string DeviceCommands::encrypt_password(const std::string& password) {
    // Note: This is a placeholder implementation
    // In production, use proper encryption like AES with secure key management
    return password; // TODO: Implement proper encryption
}

std::string DeviceCommands::decrypt_password(const std::string& encrypted) {
    // Note: This is a placeholder implementation
    return encrypted; // TODO: Implement proper decryption
}

bool DeviceCommands::test_ssh_connection(const DeviceProfile& profile) {
    // Placeholder implementation
    // In production, would use libssh2 or similar library
    std::cout << "Testing SSH connection to " << profile.hostname << ":" << profile.port << "...\n";
    std::cout << "Note: SSH connection testing not yet implemented.\n";
    std::cout << "This would test: " << profile.username << "@" << profile.hostname << "\n";
    return false; // Return false until actual implementation
}

bool DeviceCommands::test_snmp_connection(const DeviceProfile& profile) {
    // Placeholder implementation
    // In production, would use net-snmp library
    std::cout << "Testing SNMP connection to " << profile.hostname << ":" << profile.port << "...\n";
    std::cout << "Note: SNMP connection testing not yet implemented.\n";
    return false; // Return false until actual implementation
}

std::vector<std::string> DeviceCommands::collect_device_logs(const DeviceProfile& profile) {
    // Placeholder implementation
    // In production, would establish actual connections and execute commands
    std::vector<std::string> logs;

    std::cout << "Note: Actual log collection not yet implemented.\n";
    std::cout << "Would execute commands on " << profile.hostname << ":\n";
    for (const auto& cmd : profile.commands) {
        std::cout << "  > " << cmd << "\n";

        // Add some sample log lines for demonstration
        logs.push_back("Sample log line from " + profile.hostname);
        logs.push_back("Command executed: " + cmd);
    }

    return logs;
}

void DeviceCommands::show_device_help() {
    std::cout << "NetLogAI Device Management Commands\n";
    std::cout << "===================================\n\n";

    std::cout << "Device Management:\n";
    std::cout << "  device add <hostname>         Add a new network device\n";
    std::cout << "  device remove <name|id>       Remove a device\n";
    std::cout << "  device list                   List all configured devices\n";
    std::cout << "  device show <name|id>         Show device details\n";
    std::cout << "  device edit <name|id>         Edit device configuration\n";
    std::cout << "  device test <name|id>         Test device connectivity\n\n";

    std::cout << "Log Collection:\n";
    std::cout << "  fetch <name|id>               Fetch logs from a device\n";
    std::cout << "  fetch --all                   Fetch logs from all devices\n\n";

    std::cout << "Network Discovery:\n";
    std::cout << "  device discover               Auto-discover devices\n";
    std::cout << "  device scan <cidr>            Scan network range\n\n";

    std::cout << "Examples:\n";
    std::cout << "  netlogai device add 192.168.1.1 --type cisco-ios --username admin\n";
    std::cout << "  netlogai device test router1\n";
    std::cout << "  netlogai fetch router1 --output logs.txt\n";
    std::cout << "  netlogai fetch --all\n";
}

// Stub implementations for discovery (future implementation)
int DeviceCommands::discover_devices(const cli::CommandArgs& args) {
    std::cout << "Auto-discovery not yet implemented.\n";
    std::cout << "This would scan common network ranges and detect devices.\n";
    return 0;
}

int DeviceCommands::scan_network(const cli::CommandArgs& args) {
    std::cout << "Network scanning not yet implemented.\n";
    std::cout << "This would scan the specified CIDR range for network devices.\n";
    return 0;
}

int DeviceCommands::connect_device(const cli::CommandArgs& args) {
    std::cout << "Interactive device connection not yet implemented.\n";
    std::cout << "This would open an interactive session with the device.\n";
    return 0;
}

int DeviceCommands::edit_device(const cli::CommandArgs& args) {
    std::cout << "Device editing not yet implemented.\n";
    std::cout << "This would allow modifying device configurations.\n";
    return 0;
}

// GNS3 Integration Implementation
int DeviceCommands::add_gns3_device(const cli::CommandArgs& args) {
    if (args.arg_count() < 2) {
        std::cout << "Usage: netlogai device add-gns3 <device_name> <console_port> [--host <gns3_host>] [--type <device_type>]\n";
        std::cout << "Examples:\n";
        std::cout << "  netlogai device add-gns3 Router1 5000\n";
        std::cout << "  netlogai device add-gns3 Switch1 5001 --host 192.168.1.100 --type cisco-nxos\n";
        return 1;
    }

    std::string device_name = args.get_arg(0);
    int console_port;
    try {
        console_port = std::stoi(args.get_arg(1));
    } catch (const std::exception&) {
        std::cerr << "Error: Invalid console port number\n";
        return 1;
    }

    std::string gns3_host = args.get_option("host", "127.0.0.1");
    std::string device_type = args.get_option("type", "cisco-ios");

    // Test connection first
    std::cout << "Testing GNS3 console connection to " << gns3_host << ":" << console_port << "...\n";

    if (!test_gns3_console_connection(gns3_host, console_port)) {
        std::cerr << "Error: Cannot connect to GNS3 console at " << gns3_host << ":" << console_port << "\n";
        return 1;
    }

    // Create device profile
    DeviceProfile profile;
    profile.id = device_name + "_" + std::to_string(console_port);
    profile.name = device_name;
    profile.hostname = gns3_host;
    profile.port = console_port;
    profile.connection_type = DeviceConnectionType::GNS3_CONSOLE;
    profile.auth_type = DeviceAuthType::PASSWORD;
    profile.username = "";  // Most GNS3 devices don't require auth
    profile.password = "";
    profile.device_type = device_type;
    profile.timeout_seconds = 30;
    profile.enabled = true;

    // Add standard log collection commands for Cisco devices
    if (device_type == "cisco-ios" || device_type == "cisco-nxos") {
        profile.commands = {"show logging", "show version", "show interfaces"};
    }

    // Check if device already exists
    if (find_device_by_name(device_name) != nullptr) {
        std::cerr << "Error: Device with name '" << device_name << "' already exists\n";
        return 1;
    }

    device_profiles.push_back(profile);
    save_device_profiles();

    std::cout << "✅ GNS3 device '" << device_name << "' added successfully\n";
    std::cout << "   Console: " << gns3_host << ":" << console_port << "\n";
    std::cout << "   Type: " << device_type << "\n";
    std::cout << "   Use 'netlogai fetch " << device_name << "' to collect logs\n";

    return 0;
}

int DeviceCommands::discover_gns3_lab(const cli::CommandArgs& args) {
    std::string gns3_host = args.get_option("host", "127.0.0.1");

    std::cout << "🔍 Discovering GNS3 devices at " << gns3_host << "...\n";

    auto discovered_devices = discover_gns3_devices(gns3_host);

    if (discovered_devices.empty()) {
        std::cout << "No GNS3 devices found or GNS3 server not running.\n";
        std::cout << "Make sure GNS3 is running and has active devices.\n";
        return 1;
    }

    std::cout << "\n📱 Found " << discovered_devices.size() << " GNS3 device(s):\n";
    std::cout << std::string(60, '=') << "\n";

    for (const auto& device : discovered_devices) {
        std::cout << "Device: " << device.name << "\n";
        std::cout << "  Console Port: " << device.port << "\n";
        std::cout << "  Type: " << device.device_type << "\n";
        std::cout << "  Status: " << (device.enabled ? "Active" : "Inactive") << "\n";

        if (args.has_flag("add")) {
            // Auto-add discovered devices
            if (find_device_by_name(device.name) == nullptr) {
                device_profiles.push_back(device);
                std::cout << "  ✅ Added to NetLogAI\n";
            } else {
                std::cout << "  ⚠️  Already exists in NetLogAI\n";
            }
        } else {
            std::cout << "  💡 Use --add flag to automatically add discovered devices\n";
        }
        std::cout << "\n";
    }

    if (args.has_flag("add")) {
        save_device_profiles();
        std::cout << "Device profiles saved. Use 'netlogai device list' to see all devices.\n";
    }

    return 0;
}

int DeviceCommands::fetch_gns3_logs(const cli::CommandArgs& args) {
    if (args.arg_count() < 1) {
        std::cout << "Usage: netlogai fetch gns3 <device_name> [--lines <count>]\n";
        std::cout << "Examples:\n";
        std::cout << "  netlogai fetch gns3 Router1\n";
        std::cout << "  netlogai fetch gns3 all --lines 500\n";
        return 1;
    }

    std::string device_name = args.get_arg(0);
    int max_lines = std::stoi(args.get_option("lines", "1000"));

    if (device_name == "all") {
        // Fetch from all GNS3 devices
        std::cout << "🔄 Fetching logs from all GNS3 devices...\n";

        int success_count = 0;
        int total_count = 0;

        for (const auto& profile : device_profiles) {
            if (profile.connection_type == DeviceConnectionType::GNS3_CONSOLE && profile.enabled) {
                total_count++;
                std::cout << "\n📡 Fetching logs from " << profile.name << "...\n";

                auto logs = collect_gns3_device_logs(profile);
                if (!logs.empty()) {
                    std::cout << "✅ Collected " << logs.size() << " log entries from " << profile.name << "\n";

                    // Display recent logs
                    int display_count = std::min(5, static_cast<int>(logs.size()));
                    std::cout << "Recent entries:\n";
                    for (int i = logs.size() - display_count; i < static_cast<int>(logs.size()); ++i) {
                        std::cout << "  " << logs[i] << "\n";
                    }
                    success_count++;
                } else {
                    std::cout << "⚠️  No logs collected from " << profile.name << "\n";
                }
            }
        }

        std::cout << "\n📊 Summary: Collected logs from " << success_count << "/" << total_count << " GNS3 devices\n";
        return success_count > 0 ? 0 : 1;
    } else {
        // Fetch from specific device
        auto* profile = find_device_by_name(device_name);
        if (!profile) {
            std::cerr << "Error: Device '" << device_name << "' not found\n";
            return 1;
        }

        if (profile->connection_type != DeviceConnectionType::GNS3_CONSOLE) {
            std::cerr << "Error: Device '" << device_name << "' is not a GNS3 console device\n";
            return 1;
        }

        std::cout << "📡 Fetching logs from " << device_name << " via GNS3 console...\n";

        auto logs = collect_gns3_device_logs(*profile);
        if (logs.empty()) {
            std::cout << "⚠️  No logs collected from " << device_name << "\n";
            return 1;
        }

        std::cout << "✅ Collected " << logs.size() << " log entries\n";
        std::cout << "\n📄 Recent log entries:\n";
        std::cout << std::string(80, '-') << "\n";

        int display_count = std::min(max_lines, static_cast<int>(logs.size()));
        for (int i = logs.size() - display_count; i < static_cast<int>(logs.size()); ++i) {
            std::cout << logs[i] << "\n";
        }

        return 0;
    }
}

// Helper function implementations
bool DeviceCommands::test_gns3_console_connection(const std::string& host, int port) {
    try {
        networking::TelnetClient client(5); // 5 second timeout for testing
        auto result = client.connect(host, port);
        client.disconnect();
        return result.success;
    } catch (const std::exception&) {
        return false;
    }
}

std::vector<DeviceProfile> DeviceCommands::discover_gns3_devices(const std::string& gns3_host) {
    std::vector<DeviceProfile> discovered_devices;

    // Use the GNS3TelnetHelper to discover console ports
    auto console_ports = networking::GNS3TelnetHelper::discover_gns3_console_ports(gns3_host);

    for (int port : console_ports) {
        // Detect device type for each port
        std::string device_type = networking::GNS3TelnetHelper::detect_device_type_via_console(gns3_host, port);

        if (device_type != "unknown") {
            DeviceProfile profile;
            profile.id = "gns3_device_" + std::to_string(port);
            profile.name = "GNS3-Device-" + std::to_string(port);
            profile.hostname = gns3_host;
            profile.port = port;
            profile.connection_type = DeviceConnectionType::GNS3_CONSOLE;
            profile.auth_type = DeviceAuthType::PASSWORD;
            profile.username = "";
            profile.password = "";
            profile.device_type = device_type;
            profile.timeout_seconds = 30;
            profile.enabled = true;

            // Add standard commands based on device type
            if (device_type == "cisco-ios" || device_type == "cisco-nxos") {
                profile.commands = {"show logging", "show version", "show interfaces"};
            }

            discovered_devices.push_back(profile);
        }
    }

    return discovered_devices;
}

std::vector<std::string> DeviceCommands::collect_gns3_device_logs(const DeviceProfile& profile) {
    std::vector<std::string> logs;

    try {
        networking::TelnetClient client(profile.timeout_seconds);
        client.set_debug_mode(false); // Set to true for debugging

        std::cout << "  Connecting to console at " << profile.hostname << ":" << profile.port << "...\n";

        auto connect_result = client.connect(profile.hostname, profile.port);
        if (!connect_result.success) {
            std::cerr << "  ❌ Connection failed: " << connect_result.error_message << "\n";
            return logs;
        }

        std::cout << "  ✅ Connected to device console\n";

        // Collect logs using GNS3-specific method
        auto device_logs = client.gns3_collect_logs(profile.device_type);

        if (!device_logs.empty()) {
            std::cout << "  📄 Successfully collected " << device_logs.size() << " log entries\n";
            logs = device_logs;
        } else {
            std::cout << "  ⚠️  No logs available from device\n";
        }

        client.disconnect();

    } catch (const std::exception& e) {
        std::cerr << "  ❌ Exception during log collection: " << e.what() << "\n";
    }

    return logs;
}

bool DeviceCommands::test_telnet_connection(const DeviceProfile& profile) {
    if (profile.connection_type == DeviceConnectionType::TELNET ||
        profile.connection_type == DeviceConnectionType::GNS3_CONSOLE) {
        return test_gns3_console_connection(profile.hostname, profile.port);
    }
    return false;
}

} // namespace netlogai::commands