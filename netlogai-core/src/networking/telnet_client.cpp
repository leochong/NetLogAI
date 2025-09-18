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

    int result = select(static_cast<int>(socket_ + 1), &read_fds, nullptr, nullptr, &timeout);
    return result > 0;
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

TelnetCommandResult TelnetClient::send_command(const std::string& command,
                                             const std::string& expected_prompt,
                                             int timeout_ms) {
    TelnetCommandResult result;
    auto start_time = std::chrono::steady_clock::now();

    if (!connected_) {
        result.error_message = "Not connected";
        return result;
    }

    debug_log("Sending command: " + command);

    // Send command with newline
    if (!send_data(command + "\r\n")) {
        result.error_message = "Failed to send command";
        return result;
    }

    // Wait for response and prompt
    std::string output = receive_until_prompt(expected_prompt, timeout_ms);

    if (output.empty()) {
        result.error_message = "Command timeout or no response";
        return result;
    }

    // Clean up the output
    result.output = clean_output(output);
    result.success = true;

    auto end_time = std::chrono::steady_clock::now();
    result.execution_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    debug_log("Command completed in " + std::to_string(result.execution_time.count()) + "ms");
    return result;
}

std::string TelnetClient::receive_until_prompt(const std::string& expected_prompt, int timeout_ms) {
    std::string accumulated_output;
    auto start_time = std::chrono::steady_clock::now();

    while (true) {
        auto elapsed = std::chrono::steady_clock::now() - start_time;
        if (elapsed > std::chrono::milliseconds(timeout_ms)) {
            debug_log("Timeout waiting for prompt: " + expected_prompt);
            break;
        }

        std::string data = receive_data(500);
        if (data.empty()) {
            continue;
        }

        // Handle telnet negotiation
        std::string clean_data;
        handle_telnet_negotiation(data, clean_data);
        accumulated_output += clean_data;

        // Check for prompt in the last few lines
        std::istringstream stream(accumulated_output);
        std::string line;
        std::vector<std::string> lines;

        while (std::getline(stream, line)) {
            lines.push_back(line);
        }

        // Check last few lines for prompt
        for (int i = std::max(0, static_cast<int>(lines.size()) - 3); i < static_cast<int>(lines.size()); ++i) {
            if (is_expected_prompt(lines[i], expected_prompt)) {
                debug_log("Found expected prompt: " + lines[i]);
                return accumulated_output;
            }
        }
    }

    return accumulated_output;
}

void TelnetClient::handle_telnet_negotiation(const std::string& data, std::string& output) {
    output.clear();

    for (size_t i = 0; i < data.length(); ++i) {
        unsigned char c = static_cast<unsigned char>(data[i]);

        if (c == static_cast<unsigned char>(TelnetCommand::IAC) && i + 2 < data.length()) {
            // Handle telnet command
            unsigned char cmd = static_cast<unsigned char>(data[i + 1]);
            unsigned char option = static_cast<unsigned char>(data[i + 2]);

            process_telnet_option(static_cast<TelnetCommand>(cmd), static_cast<TelnetOption>(option));
            i += 2; // Skip the command and option bytes
        } else if (c >= 32 || c == '\n' || c == '\r' || c == '\t') {
            // Printable character or whitespace
            output += static_cast<char>(c);
        }
        // Ignore other control characters
    }
}

void TelnetClient::send_telnet_command(TelnetCommand cmd, TelnetOption option) {
    if (!connected_) return;

    std::string telnet_cmd;
    telnet_cmd += static_cast<char>(TelnetCommand::IAC);
    telnet_cmd += static_cast<char>(cmd);
    telnet_cmd += static_cast<char>(option);

    send_data(telnet_cmd);
    debug_log("Sent telnet command: " + std::to_string(static_cast<int>(cmd)) + " " + std::to_string(static_cast<int>(option)));
}

void TelnetClient::process_telnet_option(TelnetCommand cmd, TelnetOption option) {
    debug_log("Processing telnet option: " + std::to_string(static_cast<int>(cmd)) + " " + std::to_string(static_cast<int>(option)));

    switch (cmd) {
        case TelnetCommand::DO:
            if (option == TelnetOption::ECHO || option == TelnetOption::SGA) {
                send_telnet_command(TelnetCommand::WILL, option);
            } else {
                send_telnet_command(TelnetCommand::WONT, option);
            }
            break;
        case TelnetCommand::WILL:
            if (option == TelnetOption::ECHO || option == TelnetOption::SGA) {
                send_telnet_command(TelnetCommand::DO, option);
            } else {
                send_telnet_command(TelnetCommand::DONT, option);
            }
            break;
        case TelnetCommand::DONT:
        case TelnetCommand::WONT:
            // Acknowledge
            break;
        default:
            break;
    }
}

std::string TelnetClient::clean_output(const std::string& raw_output) {
    std::string cleaned = raw_output;

    // Remove carriage returns
    cleaned.erase(std::remove(cleaned.begin(), cleaned.end(), '\r'), cleaned.end());

    // Remove ANSI escape sequences
    std::regex ansi_regex(R"(\x1b\[[0-9;]*[mGKHf])");
    cleaned = std::regex_replace(cleaned, ansi_regex, "");

    // Remove excessive newlines
    std::regex multiple_newlines(R"(\n{3,})");
    cleaned = std::regex_replace(cleaned, multiple_newlines, "\n\n");

    return cleaned;
}

bool TelnetClient::is_expected_prompt(const std::string& line, const std::string& expected) {
    if (expected == "#") {
        return is_cisco_prompt(line);
    }

    return line.find(expected) != std::string::npos;
}

bool TelnetClient::is_cisco_prompt(const std::string& line) {
    // Remove leading/trailing whitespace
    std::string trimmed = line;
    trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
    trimmed.erase(trimmed.find_last_not_of(" \t\n\r") + 1);

    if (trimmed.empty()) return false;

    // Check for common Cisco prompts
    return (trimmed.back() == '#' || trimmed.back() == '>') &&
           trimmed.find_first_of(" \t") == std::string::npos;
}

// Cisco IOS specific operations
TelnetCommandResult TelnetClient::cisco_login(const std::string& username, const std::string& password) {
    TelnetCommandResult result;

    if (!connected_) {
        result.error_message = "Not connected";
        return result;
    }

    // Wait for login prompt or check if already logged in
    std::string initial_data = receive_data(3000);

    if (initial_data.find("Username:") != std::string::npos ||
        initial_data.find("login:") != std::string::npos) {

        if (!username.empty()) {
            send_data(username + "\r\n");
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        if (!password.empty() &&
            (initial_data.find("Password:") != std::string::npos || receive_data(2000).find("Password:") != std::string::npos)) {
            send_data(password + "\r\n");
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }

    // Check if we got a prompt
    std::string prompt_check = receive_data(2000);
    if (is_cisco_prompt(prompt_check) || prompt_check.find(">") != std::string::npos) {
        result.success = true;
        result.output = prompt_check;
        current_prompt_ = extract_hostname_from_prompt(prompt_check);
    } else {
        result.error_message = "Login failed or no valid prompt received";
        result.output = prompt_check;
    }

    return result;
}

TelnetCommandResult TelnetClient::cisco_enable(const std::string& enable_password) {
    if (privileged_mode_) {
        TelnetCommandResult result;
        result.success = true;
        result.output = "Already in privileged mode";
        return result;
    }

    auto result = send_command("enable", "#", 5000);
    if (result.success) {
        if (result.output.find("Password:") != std::string::npos && !enable_password.empty()) {
            result = send_command(enable_password, "#", 5000);
        }

        if (result.success) {
            privileged_mode_ = true;
        }
    }

    return result;
}

std::vector<std::string> TelnetClient::cisco_show_logging(int max_lines) {
    std::vector<std::string> logs;

    if (!connected_) {
        return logs;
    }

    // Disable paging
    send_command("terminal length 0", "#", 3000);

    // Get logs
    auto result = send_command("show logging", "#", 10000);
    if (result.success) {
        std::istringstream stream(result.output);
        std::string line;
        int line_count = 0;

        while (std::getline(stream, line) && line_count < max_lines) {
            if (!line.empty() && line.find("show logging") == std::string::npos) {
                logs.push_back(line);
                line_count++;
            }
        }
    }

    return logs;
}

// GNS3 specific operations
TelnetCommandResult TelnetClient::gns3_connect_console(const std::string& gns3_host, int console_port) {
    TelnetCommandResult result;
    auto connect_result = connect(gns3_host, console_port);

    result.success = connect_result.success;
    result.error_message = connect_result.error_message;
    result.output = connect_result.success ? "Connected to GNS3 console" : "";
    result.execution_time = std::chrono::milliseconds(0);

    return result;
}

std::vector<std::string> TelnetClient::gns3_collect_logs(const std::string& device_type) {
    std::vector<std::string> logs;

    if (!connected_) {
        return logs;
    }

    // Wait for device to be ready
    if (!gns3_detect_device_ready()) {
        return logs;
    }

    // Try to login (most GNS3 devices don't require authentication by default)
    cisco_login("", "");

    // Enable privileged mode if needed
    cisco_enable("");

    // Collect logs based on device type
    if (device_type == "cisco-ios" || device_type == "cisco-nxos") {
        logs = cisco_show_logging();
    }

    return logs;
}

bool TelnetClient::gns3_detect_device_ready() {
    // Wait a bit for device boot
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // Try to get some response
    send_data("\r\n");
    std::string response = receive_data(3000);

    // Check if we get any recognizable prompt or output
    return !response.empty() && (response.find(">") != std::string::npos ||
                                response.find("#") != std::string::npos ||
                                response.find("login:") != std::string::npos);
}

std::string TelnetClient::extract_hostname_from_prompt(const std::string& prompt) {
    std::regex prompt_regex(R"((\w+)[>#])");
    std::smatch match;

    if (std::regex_search(prompt, match, prompt_regex)) {
        return match[1].str();
    }

    return "unknown";
}

void TelnetClient::debug_log(const std::string& message) {
    if (debug_mode_) {
        std::cout << "[TELNET DEBUG] " << message << std::endl;
    }
}

void TelnetClient::log_raw_data(const std::string& data, bool sent) {
    if (debug_mode_) {
        std::cout << "[TELNET " << (sent ? "SENT" : "RECV") << "] " << data.length() << " bytes" << std::endl;
    }
}

// GNS3TelnetHelper implementation
std::vector<int> GNS3TelnetHelper::discover_gns3_console_ports(const std::string& gns3_host) {
    std::vector<int> ports;

    // GNS3 typically uses ports starting from 5000
    for (int port = 5000; port <= 5100; ++port) {
        TelnetClient client(2); // Short timeout for discovery
        auto result = client.connect(gns3_host, port);

        if (result.success) {
            ports.push_back(port);
            client.disconnect();
        }
    }

    return ports;
}

std::string GNS3TelnetHelper::detect_device_type_via_console(const std::string& host, int port) {
    TelnetClient client(10);
    auto result = client.connect(host, port);

    if (!result.success) {
        return "unknown";
    }

    client.set_debug_mode(false);

    // Try to detect device type by sending a command
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    client.send_data("\r\n");
    std::string response = client.receive_data(3000);

    if (response.find("IOS") != std::string::npos || response.find("Cisco") != std::string::npos) {
        return "cisco-ios";
    } else if (response.find("NX-OS") != std::string::npos) {
        return "cisco-nxos";
    }

    return "generic";
}

} // namespace netlogai::networking