#pragma once

#include "../cli/command_line.hpp"
#include <string>
#include <vector>
#include <memory>
#include <chrono>

namespace netlogai::commands {

// AI provider configuration
enum class AIProvider {
    ANTHROPIC,
    OPENAI,
    NONE
};

// Conversation context for multi-turn interactions
struct ConversationContext {
    std::string session_id;
    std::vector<std::pair<std::string, std::string>> messages; // role, content pairs
    std::chrono::system_clock::time_point last_interaction;
    std::string current_device_context;
    std::vector<std::string> current_log_context;
    int max_context_messages;
};

// AI configuration structure
struct AIConfig {
    AIProvider provider;
    std::string api_key;
    std::string model;
    std::string base_url;
    int max_tokens;
    double temperature;
    int timeout_seconds;
    bool enabled;
};

class AICommands {
public:
    static void register_commands(cli::CommandLine& cli);

private:
    // Main AI interaction commands
    static int ask_query(const cli::CommandArgs& args);
    static int start_conversation(const cli::CommandArgs& args);
    static int analyze_logs(const cli::CommandArgs& args);
    static int explain_error(const cli::CommandArgs& args);
    static int suggest_fix(const cli::CommandArgs& args);

    // AI configuration commands
    static int setup_ai(const cli::CommandArgs& args);
    static int ai_status(const cli::CommandArgs& args);
    static int test_ai_connection(const cli::CommandArgs& args);

    // Context management
    static int show_context(const cli::CommandArgs& args);
    static int clear_context(const cli::CommandArgs& args);

    // Helper functions
    static void show_ai_help();
    static bool load_ai_config();
    static bool save_ai_config();
    static std::string get_ai_config_path();

    // Core AI integration functions
    static std::string call_claude_api(const std::string& prompt, const std::vector<std::string>& context_logs = {});
    static std::string build_network_analysis_prompt(const std::string& user_question, const std::vector<std::string>& logs);
    static std::string build_error_explanation_prompt(const std::string& error_text, const std::string& device_type);
    static std::string build_troubleshooting_prompt(const std::string& issue_description, const std::vector<std::string>& logs);

    // HTTP client functions
    static std::string make_http_request(const std::string& url, const std::string& headers, const std::string& payload);
    static std::string build_claude_request_payload(const std::string& prompt, const std::vector<std::string>& context = {});
    static std::string extract_claude_response(const std::string& json_response);

    // Context management functions
    static ConversationContext* get_or_create_context(const std::string& session_id);
    static void add_to_context(ConversationContext* context, const std::string& role, const std::string& content);
    static void load_log_context_for_device(ConversationContext* context, const std::string& device_name);
    static std::vector<std::string> get_recent_logs_for_analysis(const std::string& device_name, int max_lines = 100);

    // Prompt engineering for network operations
    static std::string get_system_prompt();
    static std::string format_logs_for_ai(const std::vector<std::string>& logs);
    static std::string sanitize_log_data(const std::string& log_content);

    // Static members
    static AIConfig ai_config;
    static std::vector<ConversationContext> active_contexts;
    static const std::string CLAUDE_API_URL;
    static const std::string CLAUDE_MODEL;
};

} // namespace netlogai::commands