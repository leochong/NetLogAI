#include "networking/telnet_client.hpp"
#include <iostream>
#include <sstream>
#include <thread>
#include <algorithm>
#include <regex>

namespace netlogai::networking {

TelnetClient::TelnetClient(int timeout_seconds)
    : socket_(INVALID_SOCKET), connected_(false), timeout_seconds_(timeout_seconds),
      terminal_type_("vt100"), debug_mode_(false), prompt_timeout_ms_(5000),
      in_config_mode_(false), privileged_mode_(false)
#ifdef _WIN32
      , wsa_initialized_(false)
#endif
{
    init_socket();
}

TelnetClient::~TelnetClient() {
    disconnect();
    cleanup_socket();
}

bool TelnetClient::init_socket() {
#ifdef _WIN32
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data_) != 0) {
        return false;
    }
    wsa_initialized_ = true;
#endif
    return true;
}

void TelnetClient::cleanup_socket() {
#ifdef _WIN32
    if (wsa_initialized_) {
        WSACleanup();
        wsa_initialized_ = false;
    }
#endif
}

TelnetConnectionResult TelnetClient::connect(const std::string& hostname, int port) {
    TelnetConnectionResult result;

    if (connected_) {
        disconnect();
    }

    debug_log("Connecting to " + hostname + ":" + std::to_string(port));

    if (!connect_socket(hostname, port)) {
        result.error_message = "Failed to establish socket connection";
        result.error_code = -1;
        return result;
    }

    connected_ = true;

    // Initial telnet negotiation
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Read any initial data and handle telnet options
    std::string initial_data = receive_data(2000);
    std::string cleaned_output;
    handle_telnet_negotiation(initial_data, cleaned_output);

    // Send basic telnet options
    send_telnet_command(TelnetCommand::DO, TelnetOption::SGA);
    send_telnet_command(TelnetCommand::WILL, TelnetOption::TTYPE);

    debug_log("Connected successfully");
    result.success = true;
    return result;
}

bool TelnetClient::connect_socket(const std::string& hostname, int port) {
    socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_ == INVALID_SOCKET) {
        return false;
    }

    // Set socket timeout
#ifdef _WIN32
    DWORD timeout = timeout_seconds_ * 1000;
    setsockopt(socket_, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    setsockopt(socket_, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
#else
    struct timeval tv;
    tv.tv_sec = timeout_seconds_;
    tv.tv_usec = 0;
    setsockopt(socket_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(socket_, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#endif

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(static_cast<u_short>(port));

    if (inet_pton(AF_INET, hostname.c_str(), &server_addr.sin_addr) <= 0) {
        closesocket(socket_);
        socket_ = INVALID_SOCKET;
        return false;
    }

    if (::connect(socket_, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == SOCKET_ERROR) {
        closesocket(socket_);
        socket_ = INVALID_SOCKET;
        return false;
    }

    return true;
}

void TelnetClient::disconnect() {
    if (connected_ && socket_ != INVALID_SOCKET) {
        debug_log("Disconnecting");
        closesocket(socket_);
        socket_ = INVALID_SOCKET;
        connected_ = false;
    }
}

bool TelnetClient::send_data(const std::string& data) {
    if (!connected_ || socket_ == INVALID_SOCKET) {
        return false;
    }

    log_raw_data(data, true);

    int bytes_sent = send(socket_, data.c_str(), static_cast<int>(data.length()), 0);
    return bytes_sent == static_cast<int>(data.length());
}

std::string TelnetClient::receive_data(int timeout_ms) {
    if (!connected_ || socket_ == INVALID_SOCKET) {
        return "";
    }

    if (!wait_for_data(timeout_ms)) {
        return "";
    }

    return read_available_data();
}

bool TelnetClient::wait_for_data(int timeout_ms) {
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(socket_, &read_fds);

    struct timeval timeout;
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;

    int result = select(static_cast<int>(socket_) + 1, &read_fds, nullptr, nullptr, &timeout);
    return result > 0 && FD_ISSET(socket_, &read_fds);
}

std::string TelnetClient::read_available_data() {
    char buffer[4096];
    int bytes_received = recv(socket_, buffer, sizeof(buffer) - 1, 0);

    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        std::string data(buffer, bytes_received);
        log_raw_data(data, false);
        return data;
    }
    return "";
}

std::string TelnetClient::receive_until_prompt(const std::string& expected_prompt, int timeout_ms) {
    auto start_time = std::chrono::steady_clock::now();
    std::string accumulated_output;

    while (true) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time).count();

        if (elapsed >= timeout_ms) {
            break;
        }

        std::string data = receive_data(100);
        if (!data.empty()) {
            std::string cleaned;
            handle_telnet_negotiation(data, cleaned);
            accumulated_output += cleaned;

            // Check if we found the expected prompt
            if (is_expected_prompt(accumulated_output, expected_prompt)) {
                break;
            }
        }
    }

    return clean_output(accumulated_output);
}

TelnetCommandResult TelnetClient::send_command(const std::string& command,
                                             const std::string& expected_prompt,
                                             int timeout_ms) {
    TelnetCommandResult result;
    auto start_time = std::chrono::steady_clock::now();

    if (!connected_) {
        result.error_message = "Not connected to device";
        return result;
    }

    debug_log("Sending command: " + command);

    // Send command with newline
    std::string cmd_with_newline = command + "\r\n";
    if (!send_data(cmd_with_newline)) {
        result.error_message = "Failed to send command";
        return result;
    }

    // Wait for response
    std::string output = receive_until_prompt(expected_prompt, timeout_ms);

    result.success = true;
    result.output = output;
    result.execution_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start_time);

    debug_log("Command completed in " + std::to_string(result.execution_time.count()) + "ms");
    return result;
}

TelnetCommandResult TelnetClient::send_command_async(const std::string& command,
                                                   std::function<void(const std::string&)> output_callback,
                                                   const std::string& expected_prompt,
                                                   int timeout_ms) {
    TelnetCommandResult result;
    auto start_time = std::chrono::steady_clock::now();

    if (!connected_) {
        result.error_message = "Not connected to device";
        return result;
    }

    // Send command
    std::string cmd_with_newline = command + "\r\n";
    if (!send_data(cmd_with_newline)) {
        result.error_message = "Failed to send command";
        return result;
    }

    // Receive data and call callback for each chunk
    std::string accumulated_output;
    while (true) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time).count();

        if (elapsed >= timeout_ms) {
            break;
        }

        std::string data = receive_data(100);
        if (!data.empty()) {
            std::string cleaned;
            handle_telnet_negotiation(data, cleaned);
            accumulated_output += cleaned;

            // Call callback with new data
            if (!cleaned.empty()) {
                output_callback(cleaned);
            }

            // Check if we found the expected prompt
            if (is_expected_prompt(accumulated_output, expected_prompt)) {
                break;
            }
        }
    }

    result.success = true;
    result.output = clean_output(accumulated_output);
    result.execution_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start_time);

    return result;
}

void TelnetClient::handle_telnet_negotiation(const std::string& data, std::string& output) {
    output.clear();

    for (size_t i = 0; i < data.length(); ++i) {
        unsigned char byte = static_cast<unsigned char>(data[i]);

        if (byte == static_cast<unsigned char>(TelnetCommand::IAC)) {
            // Handle telnet command
            if (i + 2 < data.length()) {
                TelnetCommand cmd = static_cast<TelnetCommand>(data[i + 1]);
                TelnetOption option = static_cast<TelnetOption>(data[i + 2]);

                process_telnet_option(cmd, option);
                i += 2; // Skip the command and option bytes
            }
        } else {
            // Regular character
            output += static_cast<char>(byte);
        }
    }
}

void TelnetClient::send_telnet_command(TelnetCommand cmd, TelnetOption option) {
    std::string telnet_cmd;
    telnet_cmd += static_cast<char>(TelnetCommand::IAC);
    telnet_cmd += static_cast<char>(cmd);
    telnet_cmd += static_cast<char>(option);

    send_data(telnet_cmd);
}

void TelnetClient::process_telnet_option(TelnetCommand cmd, TelnetOption option) {
    debug_log("Processing telnet option: " + std::to_string(static_cast<int>(cmd)) +
              " " + std::to_string(static_cast<int>(option)));

    switch (cmd) {
        case TelnetCommand::DO:
            // Server wants us to enable an option
            if (option == TelnetOption::TTYPE) {
                send_telnet_command(TelnetCommand::WILL, option);
            } else {
                send_telnet_command(TelnetCommand::WONT, option);
            }
            break;

        case TelnetCommand::DONT:
            // Server doesn't want us to use an option
            send_telnet_command(TelnetCommand::WONT, option);
            break;

        case TelnetCommand::WILL:
            // Server will enable an option
            if (option == TelnetOption::ECHO || option == TelnetOption::SGA) {
                send_telnet_command(TelnetCommand::DO, option);
            } else {
                send_telnet_command(TelnetCommand::DONT, option);
            }
            break;

        case TelnetCommand::WONT:
            // Server won't enable an option
            send_telnet_command(TelnetCommand::DONT, option);
            break;

        default:
            break;
    }
}

std::string TelnetClient::clean_output(const std::string& raw_output) {
    std::string cleaned = raw_output;

    // Remove common control characters
    cleaned.erase(std::remove(cleaned.begin(), cleaned.end(), '\r'), cleaned.end());

    // Remove ANSI escape sequences
    std::regex ansi_regex(R"(\x1B\[[0-9;]*[a-zA-Z])");
    cleaned = std::regex_replace(cleaned, ansi_regex, "");

    // Remove backspace sequences
    std::regex backspace_regex(R"(.\x08)");
    cleaned = std::regex_replace(cleaned, backspace_regex, "");

    return cleaned;
}

bool TelnetClient::is_cisco_prompt(const std::string& line) {
    // Look for Cisco IOS prompts like "Router>", "Router#", "Router(config)#"
    std::regex cisco_regex(R"([A-Za-z0-9\-_]+[>#]\s*$)");
    std::regex config_regex(R"([A-Za-z0-9\-_]+\([^)]+\)[>#]\s*$)");

    return std::regex_search(line, cisco_regex) || std::regex_search(line, config_regex);
}

bool TelnetClient::is_expected_prompt(const std::string& line, const std::string& expected) {
    if (expected == "#") {
        return is_cisco_prompt(line) && line.find('#') != std::string::npos;
    } else if (expected == ">") {
        return is_cisco_prompt(line) && line.find('>') != std::string::npos;
    } else {
        return line.find(expected) != std::string::npos;
    }
}

std::string TelnetClient::extract_hostname_from_prompt(const std::string& prompt) {
    std::regex hostname_regex(R"(([A-Za-z0-9\-_]+)[>#])");
    std::smatch match;

    if (std::regex_search(prompt, match, hostname_regex)) {
        return match[1].str();
    }

    return "";
}

void TelnetClient::debug_log(const std::string& message) {
    if (debug_mode_) {
        std::cout << "[TelnetClient] " << message << std::endl;
    }
}

void TelnetClient::log_raw_data(const std::string& data, bool sent) {
    if (debug_mode_) {
        std::cout << "[" << (sent ? "SENT" : "RECV") << "] ";
        for (char c : data) {
            if (std::isprint(c)) {
                std::cout << c;
            } else {
                std::cout << "\\x" << std::hex << std::setw(2) << std::setfill('0')
                         << static_cast<unsigned char>(c) << std::dec;
            }
        }
        std::cout << std::endl;
    }
}

// Cisco IOS specific operations
TelnetCommandResult TelnetClient::cisco_login(const std::string& username, const std::string& password) {
    TelnetCommandResult result;

    if (!connected_) {
        result.error_message = "Not connected to device";
        return result;
    }

    // Wait for initial prompt or login
    std::string initial_output = receive_until_prompt(":", 5000);

    if (initial_output.find("Username:") != std::string::npos ||
        initial_output.find("login:") != std::string::npos) {
        // Login required
        if (username.empty()) {
            result.error_message = "Username required but not provided";
            return result;
        }

        auto username_result = send_command(username, ":", 3000);
        if (!username_result.success) {
            result.error_message = "Failed to send username";
            return result;
        }

        auto password_result = send_command(password, ">", 3000);
        if (!password_result.success) {
            result.error_message = "Failed to send password";
            return result;
        }
    }

    // Check if we're at a prompt
    std::string prompt_check = receive_until_prompt(">", 2000);
    if (is_cisco_prompt(prompt_check)) {
        current_hostname_ = extract_hostname_from_prompt(prompt_check);
        result.success = true;
        result.output = prompt_check;
    } else {
        result.error_message = "Failed to reach Cisco prompt";
    }

    return result;
}

TelnetCommandResult TelnetClient::cisco_enable(const std::string& enable_password) {
    auto result = send_command("enable", ":", 3000);
    if (!result.success) {
        return result;
    }

    if (result.output.find("Password:") != std::string::npos) {
        result = send_command(enable_password, "#", 3000);
        if (result.success) {
            privileged_mode_ = true;
        }
    } else {
        // Already in privileged mode or no enable password required
        privileged_mode_ = true;
    }

    return result;
}

TelnetCommandResult TelnetClient::cisco_configure_terminal() {
    auto result = send_command("configure terminal", "#", 3000);
    if (result.success) {
        in_config_mode_ = true;
    }
    return result;
}

TelnetCommandResult TelnetClient::cisco_exit_config() {
    TelnetCommandResult result;

    if (in_config_mode_) {
        result = send_command("exit", "#", 3000);
        if (result.success) {
            in_config_mode_ = false;
        }
    } else {
        result.success = true;
        result.output = "Not in configuration mode";
    }

    return result;
}

std::vector<std::string> TelnetClient::cisco_show_logging(int max_lines) {
    std::vector<std::string> log_lines;

    auto result = send_command("show logging", "#", 10000);
    if (result.success) {
        std::istringstream iss(result.output);
        std::string line;
        int line_count = 0;

        while (std::getline(iss, line) && line_count < max_lines) {
            // Skip empty lines and prompts
            if (!line.empty() && !is_cisco_prompt(line) &&
                line.find("show logging") == std::string::npos) {
                log_lines.push_back(line);
                line_count++;
            }
        }
    }

    return log_lines;
}

// GNS3 specific operations
TelnetCommandResult TelnetClient::gns3_connect_console(const std::string& gns3_host, int console_port) {
    TelnetCommandResult result;
    auto connection_result = connect(gns3_host, console_port);

    result.success = connection_result.success;
    result.error_message = connection_result.error_message;
    result.output = connection_result.success ? "Connected to GNS3 console" : "";
    result.execution_time = std::chrono::milliseconds(0);

    return result;
}

std::vector<std::string> TelnetClient::gns3_collect_logs(const std::string& device_type) {
    std::vector<std::string> logs;

    if (!connected_) {
        return logs;
    }

    // Try to detect if device is ready
    if (!gns3_detect_device_ready()) {
        debug_log("Device not ready for commands");
        return logs;
    }

    if (device_type == "cisco-ios" || device_type == "cisco-nxos") {
        // Try to collect Cisco logs
        auto cisco_logs = cisco_show_logging(1000);
        logs.insert(logs.end(), cisco_logs.begin(), cisco_logs.end());
    } else {
        // Generic log collection - try common commands
        auto result = send_command("show log", "#", 5000);
        if (result.success) {
            std::istringstream iss(result.output);
            std::string line;
            while (std::getline(iss, line)) {
                if (!line.empty() && !is_cisco_prompt(line)) {
                    logs.push_back(line);
                }
            }
        }
    }

    return logs;
}

bool TelnetClient::gns3_detect_device_ready() {
    if (!connected_) {
        return false;
    }

    // Send a simple command to test responsiveness
    std::string test_data = receive_data(1000);

    // Check if we see a prompt in the data
    return is_cisco_prompt(test_data) || test_data.find("#") != std::string::npos ||
           test_data.find(">") != std::string::npos;
}

// GNS3TelnetHelper implementation
std::vector<int> GNS3TelnetHelper::discover_gns3_console_ports(const std::string& gns3_host) {
    std::vector<int> available_ports;

    // Common GNS3 console port range
    for (int port = 5000; port < 5100; ++port) {
        TelnetClient test_client(5); // 5 second timeout for testing
        auto result = test_client.connect(gns3_host, port);

        if (result.success) {
            available_ports.push_back(port);
            test_client.disconnect();
        }
    }

    return available_ports;
}

std::string GNS3TelnetHelper::detect_device_type_via_console(const std::string& host, int port) {
    TelnetClient client(10);
    auto connection_result = client.connect(host, port);

    if (!connection_result.success) {
        return "unknown";
    }

    client.set_debug_mode(false);

    // Try to get some initial output
    std::string initial_output = client.receive_data(2000);

    // Look for Cisco IOS indicators
    if (initial_output.find("IOS") != std::string::npos ||
        initial_output.find("Cisco") != std::string::npos ||
        initial_output.find("#") != std::string::npos ||
        initial_output.find(">") != std::string::npos) {

        // Try a show version command to distinguish between IOS variants
        auto version_result = client.send_command("show version", "#", 5000);
        if (version_result.success) {
            if (version_result.output.find("NX-OS") != std::string::npos) {
                client.disconnect();
                return "cisco-nxos";
            } else if (version_result.output.find("ASA") != std::string::npos) {
                client.disconnect();
                return "cisco-asa";
            } else {
                client.disconnect();
                return "cisco-ios";
            }
        }

        client.disconnect();
        return "cisco-ios"; // Default to IOS if we can't determine exactly
    }

    client.disconnect();
    return "unknown";
}

std::vector<TelnetCommandResult> GNS3TelnetHelper::execute_commands_on_multiple_devices(
    const std::vector<std::pair<std::string, int>>& devices,
    const std::vector<std::string>& commands) {

    std::vector<TelnetCommandResult> results;

    for (const auto& device : devices) {
        TelnetClient client(30);
        auto connection_result = client.connect(device.first, device.second);

        if (connection_result.success) {
            for (const auto& command : commands) {
                auto cmd_result = client.send_command(command, "#", 10000);
                results.push_back(cmd_result);
            }
            client.disconnect();
        } else {
            TelnetCommandResult error_result;
            error_result.success = false;
            error_result.error_message = "Failed to connect to " + device.first + ":" + std::to_string(device.second);
            results.push_back(error_result);
        }
    }

    return results;
}

std::vector<std::string> GNS3TelnetHelper::collect_lab_logs(const std::string& gns3_host) {
    std::vector<std::string> all_logs;

    auto console_ports = discover_gns3_console_ports(gns3_host);

    for (int port : console_ports) {
        TelnetClient client(15);
        auto connection_result = client.connect(gns3_host, port);

        if (connection_result.success) {
            std::string device_type = detect_device_type_via_console(gns3_host, port);
            auto device_logs = client.gns3_collect_logs(device_type);

            // Add device identifier to logs
            for (const auto& log : device_logs) {
                all_logs.push_back("[" + gns3_host + ":" + std::to_string(port) + "] " + log);
            }

            client.disconnect();
        }
    }

    return all_logs;
}

} // namespace netlogai::networking