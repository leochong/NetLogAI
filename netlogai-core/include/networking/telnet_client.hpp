#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <functional>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#endif

namespace netlogai::networking {

// Telnet protocol constants
enum class TelnetCommand : unsigned char {
    SE = 240,    // End of subnegotiation parameters
    NOP = 241,   // No operation
    DM = 242,    // Data Mark
    BRK = 243,   // Break
    IP = 244,    // Interrupt process
    AO = 245,    // Abort output
    AYT = 246,   // Are you there
    EC = 247,    // Erase character
    EL = 248,    // Erase line
    GA = 249,    // Go ahead
    SB = 250,    // Subnegotiation begins
    WILL = 251,  // Will option
    WONT = 252,  // Won't option
    DO = 253,    // Do option
    DONT = 254,  // Don't option
    IAC = 255    // Interpret as command
};

enum class TelnetOption : unsigned char {
    ECHO = 1,           // Echo
    SGA = 3,            // Suppress go ahead
    TTYPE = 24,         // Terminal type
    NAWS = 31,          // Negotiate about window size
    NEW_ENVIRON = 39    // New environment option
};

// Connection result
struct TelnetConnectionResult {
    bool success = false;
    std::string error_message;
    int error_code = 0;
};

// Command execution result
struct TelnetCommandResult {
    bool success = false;
    std::string output;
    std::string error_message;
    std::chrono::milliseconds execution_time{0};
};

class TelnetClient {
public:
    explicit TelnetClient(int timeout_seconds = 30);
    ~TelnetClient();

    // Connection management
    TelnetConnectionResult connect(const std::string& hostname, int port);
    void disconnect();
    bool is_connected() const { return connected_; }

    // Command execution
    TelnetCommandResult send_command(const std::string& command,
                                   const std::string& expected_prompt = "#",
                                   int timeout_ms = 5000);

    TelnetCommandResult send_command_async(const std::string& command,
                                         std::function<void(const std::string&)> output_callback,
                                         const std::string& expected_prompt = "#",
                                         int timeout_ms = 5000);

    // Raw data operations
    bool send_data(const std::string& data);
    std::string receive_data(int timeout_ms = 1000);
    std::string receive_until_prompt(const std::string& expected_prompt = "#", int timeout_ms = 5000);

    // Cisco IOS specific operations
    TelnetCommandResult cisco_login(const std::string& username = "", const std::string& password = "");
    TelnetCommandResult cisco_enable(const std::string& enable_password = "");
    TelnetCommandResult cisco_configure_terminal();
    TelnetCommandResult cisco_exit_config();
    std::vector<std::string> cisco_show_logging(int max_lines = 1000);

    // GNS3 specific operations
    TelnetCommandResult gns3_connect_console(const std::string& gns3_host, int console_port);
    std::vector<std::string> gns3_collect_logs(const std::string& device_type = "cisco-ios");
    bool gns3_detect_device_ready();

    // Configuration
    void set_terminal_type(const std::string& terminal_type) { terminal_type_ = terminal_type; }
    void set_debug_mode(bool enabled) { debug_mode_ = enabled; }
    void set_prompt_detection_timeout(int timeout_ms) { prompt_timeout_ms_ = timeout_ms; }

private:
    // Socket operations
    bool init_socket();
    void cleanup_socket();
    bool connect_socket(const std::string& hostname, int port);

    // Telnet protocol handling
    void handle_telnet_negotiation(const std::string& data, std::string& output);
    void send_telnet_command(TelnetCommand cmd, TelnetOption option = TelnetOption::ECHO);
    void process_telnet_option(TelnetCommand cmd, TelnetOption option);

    // Data processing
    std::string clean_output(const std::string& raw_output);
    bool wait_for_data(int timeout_ms);
    std::string read_available_data();

    // Prompt detection
    bool is_cisco_prompt(const std::string& line);
    bool is_expected_prompt(const std::string& line, const std::string& expected);
    std::string extract_hostname_from_prompt(const std::string& prompt);

    // Debug and logging
    void debug_log(const std::string& message);
    void log_raw_data(const std::string& data, bool sent);

private:
    SOCKET socket_;
    bool connected_;
    int timeout_seconds_;
    std::string terminal_type_;
    bool debug_mode_;
    int prompt_timeout_ms_;

    // Connection state
    std::string current_hostname_;
    std::string current_prompt_;
    bool in_config_mode_;
    bool privileged_mode_;

    // Buffer for incomplete data
    std::string receive_buffer_;

#ifdef _WIN32
    WSADATA wsa_data_;
    bool wsa_initialized_;
#endif
};

// Utility functions for GNS3 integration
class GNS3TelnetHelper {
public:
    // GNS3 console port detection
    static std::vector<int> discover_gns3_console_ports(const std::string& gns3_host = "127.0.0.1");

    // Device type detection via telnet
    static std::string detect_device_type_via_console(const std::string& host, int port);

    // Batch operations
    static std::vector<TelnetCommandResult> execute_commands_on_multiple_devices(
        const std::vector<std::pair<std::string, int>>& devices,
        const std::vector<std::string>& commands);

    // Log collection from GNS3 lab
    static std::vector<std::string> collect_lab_logs(const std::string& gns3_host = "127.0.0.1");
};

} // namespace netlogai::networking