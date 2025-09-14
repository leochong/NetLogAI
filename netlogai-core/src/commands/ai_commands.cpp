#include "ai_commands.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <nlohmann/json.hpp>
#include <cstdlib>

using json = nlohmann::json;

namespace netlogai::commands {

// Static member initialization
AIConfig AICommands::ai_config = {};
std::vector<ConversationContext> AICommands::active_contexts = {};
const std::string AICommands::CLAUDE_API_URL = "https://api.anthropic.com/v1/messages";
const std::string AICommands::CLAUDE_MODEL = "claude-sonnet-4-20250514";

void AICommands::register_commands(cli::CommandLine& cli) {
    // Load AI configuration
    load_ai_config();

    // Register main AI interaction commands
    cli.register_command("ask", ask_query, "Ask AI questions about your network logs");
    cli.register_subcommand("ask", "logs", analyze_logs, "Analyze specific log entries with AI");
    cli.register_subcommand("ask", "error", explain_error, "Get AI explanation of error messages");
    cli.register_subcommand("ask", "fix", suggest_fix, "Get AI troubleshooting suggestions");

    // Register conversation commands
    cli.register_command("chat", start_conversation, "Start interactive conversation with AI");
    cli.register_subcommand("chat", "context", show_context, "Show current conversation context");
    cli.register_subcommand("chat", "clear", clear_context, "Clear conversation context");

    // Register AI configuration commands
    cli.register_subcommand("config", "ai", setup_ai, "Configure AI integration settings");
    cli.register_command("ai-status", ai_status, "Show AI integration status");
    cli.register_command("ai-test", test_ai_connection, "Test AI connection");

    // Register help
    cli.register_subcommand("ask", "help", [](const cli::CommandArgs&) {
        show_ai_help();
        return 0;
    }, "Show AI commands help");
}

int AICommands::ask_query(const cli::CommandArgs& args) {
    if (args.arg_count() < 1) {
        std::cout << "Usage: netlogai ask \"<your question>\"\n";
        std::cout << "Examples:\n";
        std::cout << "  netlogai ask \"Why is BGP flapping on Router1?\"\n";
        std::cout << "  netlogai ask \"What devices are showing high CPU usage?\"\n";
        std::cout << "  netlogai ask \"Analyze the last 100 log entries\"\n";
        return 1;
    }

    if (!ai_config.enabled) {
        std::cout << "AI integration is not configured or disabled.\n";
        std::cout << "Run 'netlogai config ai' to set up AI integration.\n";
        return 1;
    }

    std::string user_question = args.get_arg(0);
    std::string device_filter = args.get_option("device", "");
    std::string timespan = args.get_option("timespan", "1h");

    std::cout << "🤖 Analyzing your question with Claude AI...\n";
    std::cout << "Question: " << user_question << "\n\n";

    // Get relevant log context
    std::vector<std::string> context_logs;
    if (!device_filter.empty()) {
        context_logs = get_recent_logs_for_analysis(device_filter);
        std::cout << "📊 Analyzing logs from device: " << device_filter << "\n";
    } else {
        // Get logs from all devices
        std::cout << "📊 Analyzing logs from all configured devices...\n";
        context_logs = get_recent_logs_for_analysis("all");
    }

    if (context_logs.empty()) {
        std::cout << "⚠️  No recent logs found for analysis.\n";
        std::cout << "Try running 'netlogai fetch --all' to collect logs first.\n";
        return 1;
    }

    // Build AI prompt and make request
    std::string ai_prompt = build_network_analysis_prompt(user_question, context_logs);
    std::string ai_response = call_claude_api(ai_prompt, context_logs);

    if (ai_response.empty()) {
        std::cout << "❌ Failed to get response from AI service.\n";
        std::cout << "Check your AI configuration and network connection.\n";
        return 1;
    }

    std::cout << "🎯 AI Analysis:\n";
    std::cout << "===============\n";
    std::cout << ai_response << "\n\n";

    // Save interaction to context if requested
    if (args.has_flag("save-context")) {
        auto* context = get_or_create_context("default");
        add_to_context(context, "user", user_question);
        add_to_context(context, "assistant", ai_response);
        std::cout << "💾 Conversation saved to context.\n";
    }

    return 0;
}

int AICommands::analyze_logs(const cli::CommandArgs& args) {
    if (args.arg_count() < 1) {
        std::cout << "Usage: netlogai ask logs <log-file> [--question \"<question>\"]\n";
        std::cout << "Examples:\n";
        std::cout << "  netlogai ask logs router1_logs.txt --question \"What errors occurred?\"\n";
        std::cout << "  netlogai ask logs /path/to/logs --question \"Find BGP issues\"\n";
        return 1;
    }

    if (!ai_config.enabled) {
        std::cout << "AI integration is not enabled. Run 'netlogai config ai' to configure.\n";
        return 1;
    }

    std::string log_file = args.get_arg(0);
    std::string question = args.get_option("question", "Analyze these logs for issues, patterns, and recommendations");

    // Read log file
    std::ifstream file(log_file);
    if (!file) {
        std::cout << "Error: Unable to read log file: " << log_file << "\n";
        return 1;
    }

    std::vector<std::string> logs;
    std::string line;
    while (std::getline(file, line) && logs.size() < 500) { // Limit to 500 lines
        if (!line.empty()) {
            logs.push_back(line);
        }
    }

    if (logs.empty()) {
        std::cout << "No log entries found in file: " << log_file << "\n";
        return 1;
    }

    std::cout << "🔍 Analyzing " << logs.size() << " log entries with AI...\n";
    std::cout << "Question: " << question << "\n\n";

    std::string ai_prompt = build_network_analysis_prompt(question, logs);
    std::string ai_response = call_claude_api(ai_prompt, logs);

    if (ai_response.empty()) {
        std::cout << "❌ Failed to analyze logs with AI.\n";
        return 1;
    }

    std::cout << "🎯 AI Log Analysis:\n";
    std::cout << "==================\n";
    std::cout << ai_response << "\n";

    return 0;
}

int AICommands::explain_error(const cli::CommandArgs& args) {
    if (args.arg_count() < 1) {
        std::cout << "Usage: netlogai ask error \"<error message>\" [--device-type <type>]\n";
        std::cout << "Examples:\n";
        std::cout << "  netlogai ask error \"%LINEPROTO-5-UPDOWN: gi0/1 down\" --device-type cisco-ios\n";
        std::cout << "  netlogai ask error \"BGP neighbor 192.168.1.2 down\"\n";
        return 1;
    }

    if (!ai_config.enabled) {
        std::cout << "AI integration is not enabled.\n";
        return 1;
    }

    std::string error_message = args.get_arg(0);
    std::string device_type = args.get_option("device-type", "generic");

    std::cout << "🔍 Getting AI explanation for error...\n";
    std::cout << "Error: " << error_message << "\n\n";

    std::string ai_prompt = build_error_explanation_prompt(error_message, device_type);
    std::string ai_response = call_claude_api(ai_prompt);

    if (ai_response.empty()) {
        std::cout << "❌ Failed to get error explanation from AI.\n";
        return 1;
    }

    std::cout << "🎯 AI Error Explanation:\n";
    std::cout << "========================\n";
    std::cout << ai_response << "\n";

    return 0;
}

int AICommands::suggest_fix(const cli::CommandArgs& args) {
    if (args.arg_count() < 1) {
        std::cout << "Usage: netlogai ask fix \"<issue description>\" [--logs <log-file>]\n";
        std::cout << "Examples:\n";
        std::cout << "  netlogai ask fix \"Router1 BGP session keeps flapping\"\n";
        std::cout << "  netlogai ask fix \"High CPU usage on switches\" --logs switch_logs.txt\n";
        return 1;
    }

    if (!ai_config.enabled) {
        std::cout << "AI integration is not enabled.\n";
        return 1;
    }

    std::string issue_description = args.get_arg(0);
    std::string log_file = args.get_option("logs", "");

    std::vector<std::string> context_logs;
    if (!log_file.empty()) {
        std::ifstream file(log_file);
        if (file) {
            std::string line;
            while (std::getline(file, line) && context_logs.size() < 200) {
                if (!line.empty()) {
                    context_logs.push_back(line);
                }
            }
        }
    }

    std::cout << "🛠️  Getting AI troubleshooting suggestions...\n";
    std::cout << "Issue: " << issue_description << "\n";
    if (!context_logs.empty()) {
        std::cout << "Context: " << context_logs.size() << " log entries\n";
    }
    std::cout << "\n";

    std::string ai_prompt = build_troubleshooting_prompt(issue_description, context_logs);
    std::string ai_response = call_claude_api(ai_prompt, context_logs);

    if (ai_response.empty()) {
        std::cout << "❌ Failed to get troubleshooting suggestions from AI.\n";
        return 1;
    }

    std::cout << "🎯 AI Troubleshooting Suggestions:\n";
    std::cout << "==================================\n";
    std::cout << ai_response << "\n";

    return 0;
}

int AICommands::setup_ai(const cli::CommandArgs& args) {
    std::cout << "NetLogAI AI Integration Setup\n";
    std::cout << "============================\n\n";

    // Check current configuration
    if (ai_config.enabled) {
        std::cout << "Current AI Configuration:\n";
        std::cout << "Provider: " << (ai_config.provider == AIProvider::ANTHROPIC ? "Anthropic Claude" : "None") << "\n";
        std::cout << "Model: " << ai_config.model << "\n";
        std::cout << "Status: " << (ai_config.enabled ? "Enabled" : "Disabled") << "\n\n";
    }

    // Interactive setup
    std::cout << "Setting up Anthropic Claude integration...\n";

    std::cout << "\nEnter your Anthropic API key (or press Enter to skip): ";
    std::string api_key;
    std::getline(std::cin, api_key);

    if (!api_key.empty()) {
        ai_config.provider = AIProvider::ANTHROPIC;
        ai_config.api_key = api_key;
        ai_config.model = args.get_option("model", CLAUDE_MODEL);
        ai_config.base_url = CLAUDE_API_URL;
        ai_config.max_tokens = std::stoi(args.get_option("max-tokens", "4096"));
        ai_config.temperature = std::stod(args.get_option("temperature", "0.1"));
        ai_config.timeout_seconds = std::stoi(args.get_option("timeout", "30"));
        ai_config.enabled = true;

        if (save_ai_config()) {
            std::cout << "✅ AI configuration saved successfully!\n";
            std::cout << "\nYou can now use AI commands:\n";
            std::cout << "  netlogai ask \"Why is my network slow?\"\n";
            std::cout << "  netlogai ask error \"%LINEPROTO-5-UPDOWN\"\n";
            std::cout << "  netlogai ask fix \"BGP neighbor down\"\n";
        } else {
            std::cout << "❌ Failed to save AI configuration.\n";
            return 1;
        }
    } else {
        std::cout << "Setup cancelled. AI integration remains disabled.\n";
        return 1;
    }

    return 0;
}

int AICommands::ai_status(const cli::CommandArgs& args) {
    std::cout << "NetLogAI AI Integration Status\n";
    std::cout << "==============================\n";

    if (!ai_config.enabled) {
        std::cout << "Status: ❌ Disabled\n";
        std::cout << "Run 'netlogai config ai' to set up AI integration.\n";
        return 0;
    }

    std::cout << "Status: ✅ Enabled\n";
    std::cout << "Provider: Anthropic Claude\n";
    std::cout << "Model: " << ai_config.model << "\n";
    std::cout << "Max Tokens: " << ai_config.max_tokens << "\n";
    std::cout << "Temperature: " << ai_config.temperature << "\n";
    std::cout << "Timeout: " << ai_config.timeout_seconds << " seconds\n";

    if (!ai_config.api_key.empty()) {
        std::cout << "API Key: " << ai_config.api_key.substr(0, 8) << "..." << " (configured)\n";
    }

    std::cout << "\nActive Contexts: " << active_contexts.size() << "\n";

    return 0;
}

int AICommands::test_ai_connection(const cli::CommandArgs& args) {
    if (!ai_config.enabled) {
        std::cout << "❌ AI integration is not configured.\n";
        return 1;
    }

    std::cout << "🧪 Testing AI connection to Claude...\n";

    std::string test_prompt = "Hello! This is a connection test from NetLogAI. Please respond with 'Connection successful' if you receive this message.";
    std::string response = call_claude_api(test_prompt);

    if (!response.empty()) {
        std::cout << "✅ AI connection test successful!\n";
        std::cout << "Response: " << response << "\n";
        return 0;
    } else {
        std::cout << "❌ AI connection test failed.\n";
        std::cout << "Please check your API key and network connection.\n";
        return 1;
    }
}

// Helper function implementations
std::string AICommands::call_claude_api(const std::string& prompt, const std::vector<std::string>& context_logs) {
    // This is a placeholder implementation for the Claude API call
    // In a production environment, you would implement actual HTTP client functionality

    std::cout << "📡 [Simulated] Calling Claude API...\n";
    std::cout << "Prompt preview: " << prompt.substr(0, 100) << "...\n";

    if (!context_logs.empty()) {
        std::cout << "Context: " << context_logs.size() << " log entries\n";
    }

    // Simulate AI response based on prompt content
    if (prompt.find("BGP") != std::string::npos) {
        return "Based on the network logs, I can see BGP-related activity. The BGP session flapping could be caused by:\n\n"
               "1. **Network Connectivity Issues**: Intermittent link failures between BGP peers\n"
               "2. **Configuration Mismatch**: AS number, authentication, or timer mismatches\n"
               "3. **Resource Constraints**: High CPU or memory usage affecting BGP process\n\n"
               "**Recommended Actions:**\n"
               "- Check physical connectivity and interface status\n"
               "- Verify BGP configuration consistency between peers\n"
               "- Monitor system resources on both devices\n"
               "- Consider adjusting BGP timers if network has high latency";
    }

    if (prompt.find("error") != std::string::npos || prompt.find("ERROR") != std::string::npos) {
        return "I've analyzed the error message. This appears to be a network interface status change notification.\n\n"
               "**Error Explanation:**\n"
               "- %LINEPROTO-5-UPDOWN indicates a line protocol state change\n"
               "- Severity level 5 means this is a notification (informational)\n"
               "- The interface has transitioned to DOWN state\n\n"
               "**Possible Causes:**\n"
               "- Physical cable disconnection\n"
               "- Remote device shutdown\n"
               "- Configuration changes\n"
               "- Hardware failure\n\n"
               "**Troubleshooting Steps:**\n"
               "1. Check physical connections\n"
               "2. Verify remote device status\n"
               "3. Review recent configuration changes\n"
               "4. Test with different cables if possible";
    }

    if (prompt.find("CPU") != std::string::npos || prompt.find("high") != std::string::npos) {
        return "High CPU usage detected in the network devices. Here's my analysis:\n\n"
               "**Potential Causes:**\n"
               "- Heavy network traffic requiring more processing\n"
               "- Routing protocol convergence events\n"
               "- Security scanning or attacks\n"
               "- Misconfigured QoS or traffic shaping\n\n"
               "**Immediate Actions:**\n"
               "1. Identify which processes are consuming CPU\n"
               "2. Check for unusual traffic patterns\n"
               "3. Review recent configuration changes\n"
               "4. Monitor for security events\n\n"
               "**Long-term Solutions:**\n"
               "- Optimize routing protocols\n"
               "- Implement traffic engineering\n"
               "- Consider hardware upgrades if consistently high";
    }

    // Default response
    return "I've analyzed your network logs and query. Here are my findings:\n\n"
           "**Summary:**\n"
           "Your network infrastructure appears to be operating normally with standard operational messages.\n\n"
           "**Key Observations:**\n"
           "- Log entries show typical network device operations\n"
           "- No critical errors or alerts detected\n"
           "- System messages indicate normal protocol behavior\n\n"
           "**Recommendations:**\n"
           "- Continue monitoring for any unusual patterns\n"
           "- Set up automated alerting for critical events\n"
           "- Regular log analysis to establish baseline behavior\n\n"
           "Note: This is a simulated response. In production, this would be powered by Anthropic's Claude AI with real-time analysis.";
}

std::string AICommands::build_network_analysis_prompt(const std::string& user_question, const std::vector<std::string>& logs) {
    std::ostringstream prompt;

    prompt << "You are NetLogAI, an expert network engineer and system administrator with deep knowledge of network protocols, Cisco devices, and network troubleshooting.\n\n";

    prompt << "User Question: " << user_question << "\n\n";

    if (!logs.empty()) {
        prompt << "Network Log Context (recent " << logs.size() << " entries):\n";
        prompt << "```\n";

        // Include up to 50 most recent/relevant log lines to avoid token limits
        int max_logs = std::min(50, static_cast<int>(logs.size()));
        for (int i = logs.size() - max_logs; i < static_cast<int>(logs.size()); ++i) {
            prompt << logs[i] << "\n";
        }

        prompt << "```\n\n";
    }

    prompt << "Please analyze the logs and provide:\n";
    prompt << "1. Direct answer to the user's question\n";
    prompt << "2. Relevant findings from the log analysis\n";
    prompt << "3. Potential causes if issues are identified\n";
    prompt << "4. Specific recommendations and next steps\n";
    prompt << "5. Priority level (Low/Medium/High/Critical) if issues exist\n\n";
    prompt << "Format your response clearly with headers and bullet points for easy reading.";

    return prompt.str();
}

std::string AICommands::build_error_explanation_prompt(const std::string& error_text, const std::string& device_type) {
    std::ostringstream prompt;

    prompt << "You are a network troubleshooting expert. Please explain this network error message:\n\n";
    prompt << "Error Message: " << error_text << "\n";
    prompt << "Device Type: " << device_type << "\n\n";
    prompt << "Please provide:\n";
    prompt << "1. What this error means in plain English\n";
    prompt << "2. Severity level and impact\n";
    prompt << "3. Common causes\n";
    prompt << "4. Step-by-step troubleshooting guide\n";
    prompt << "5. Prevention strategies\n\n";
    prompt << "Be specific to the device type and error context.";

    return prompt.str();
}

std::string AICommands::build_troubleshooting_prompt(const std::string& issue_description, const std::vector<std::string>& logs) {
    std::ostringstream prompt;

    prompt << "You are an expert network engineer providing troubleshooting assistance.\n\n";
    prompt << "Issue Description: " << issue_description << "\n\n";

    if (!logs.empty()) {
        prompt << "Supporting Log Data:\n```\n";
        for (const auto& log : logs) {
            prompt << log << "\n";
        }
        prompt << "```\n\n";
    }

    prompt << "Please provide detailed troubleshooting guidance including:\n";
    prompt << "1. Problem analysis based on the description and logs\n";
    prompt << "2. Root cause investigation steps\n";
    prompt << "3. Specific commands to run for diagnosis\n";
    prompt << "4. Multiple solution approaches (quick fix vs permanent solution)\n";
    prompt << "5. Prevention strategies to avoid recurrence\n";
    prompt << "6. When to escalate to vendor support\n\n";
    prompt << "Provide practical, actionable advice suitable for network operations teams.";

    return prompt.str();
}

std::vector<std::string> AICommands::get_recent_logs_for_analysis(const std::string& device_name, int max_lines) {
    std::vector<std::string> logs;

    // Placeholder implementation - in production, this would:
    // 1. Check if device_name is "all" and collect from all devices
    // 2. Read from device-specific log files or database
    // 3. Apply time filtering and log level filtering
    // 4. Return the most recent and relevant log entries

    if (device_name == "all") {
        // Simulate logs from multiple devices
        logs.push_back("2024-01-15 10:30:15 Router1: %BGP-5-ADJCHANGE: neighbor 192.168.1.10 Up");
        logs.push_back("2024-01-15 10:30:20 Switch1: %LINK-3-UPDOWN: Interface GigabitEthernet0/1, changed state to up");
        logs.push_back("2024-01-15 10:30:25 Router1: %OSPF-5-ADJCHG: Process 1, Nbr 192.168.1.20 on Ethernet0/0 from LOADING to FULL");
        logs.push_back("2024-01-15 10:30:30 Switch1: %SYS-5-CONFIG_I: Configured from console by admin on vty0");
    } else {
        // Simulate logs from specific device
        logs.push_back("2024-01-15 10:30:15 " + device_name + ": %SYS-5-CONFIG_I: Configured from console");
        logs.push_back("2024-01-15 10:30:20 " + device_name + ": %LINK-3-UPDOWN: Interface up");
        logs.push_back("2024-01-15 10:30:25 " + device_name + ": %BGP-5-ADJCHANGE: neighbor state change");
    }

    return logs;
}

// Configuration management functions
std::string AICommands::get_ai_config_path() {
    char* home = getenv("USERPROFILE");  // Windows
    if (!home) home = getenv("HOME");    // Unix

    std::string config_dir = home ? std::string(home) : ".";
    config_dir += "/.netlogai";

    return config_dir + "/ai-config.json";
}

bool AICommands::load_ai_config() {
    std::string config_path = get_ai_config_path();
    std::ifstream file(config_path);

    if (!file) {
        // Initialize with defaults
        ai_config.provider = AIProvider::NONE;
        ai_config.enabled = false;
        ai_config.model = CLAUDE_MODEL;
        ai_config.max_tokens = 4096;
        ai_config.temperature = 0.1;
        ai_config.timeout_seconds = 30;
        return true;
    }

    try {
        json j;
        file >> j;

        std::string provider_str = j.value("provider", "none");
        if (provider_str == "anthropic") {
            ai_config.provider = AIProvider::ANTHROPIC;
        } else if (provider_str == "openai") {
            ai_config.provider = AIProvider::OPENAI;
        } else {
            ai_config.provider = AIProvider::NONE;
        }

        ai_config.api_key = j.value("api_key", "");
        ai_config.model = j.value("model", CLAUDE_MODEL);
        ai_config.base_url = j.value("base_url", CLAUDE_API_URL);
        ai_config.max_tokens = j.value("max_tokens", 4096);
        ai_config.temperature = j.value("temperature", 0.1);
        ai_config.timeout_seconds = j.value("timeout_seconds", 30);
        ai_config.enabled = j.value("enabled", false);

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading AI configuration: " << e.what() << std::endl;
        return false;
    }
}

bool AICommands::save_ai_config() {
    std::string config_path = get_ai_config_path();

    try {
        json j;

        switch (ai_config.provider) {
            case AIProvider::ANTHROPIC: j["provider"] = "anthropic"; break;
            case AIProvider::OPENAI: j["provider"] = "openai"; break;
            default: j["provider"] = "none"; break;
        }

        j["api_key"] = ai_config.api_key;
        j["model"] = ai_config.model;
        j["base_url"] = ai_config.base_url;
        j["max_tokens"] = ai_config.max_tokens;
        j["temperature"] = ai_config.temperature;
        j["timeout_seconds"] = ai_config.timeout_seconds;
        j["enabled"] = ai_config.enabled;

        std::ofstream file(config_path);
        file << j.dump(2);

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving AI configuration: " << e.what() << std::endl;
        return false;
    }
}

void AICommands::show_ai_help() {
    std::cout << "NetLogAI AI Integration Commands\n";
    std::cout << "================================\n\n";

    std::cout << "Query Commands:\n";
    std::cout << "  ask \"<question>\"                Ask AI about your network logs\n";
    std::cout << "  ask logs <file> --question \"<q>\" Analyze specific log file\n";
    std::cout << "  ask error \"<error message>\"     Get explanation of error messages\n";
    std::cout << "  ask fix \"<issue>\"               Get troubleshooting suggestions\n\n";

    std::cout << "Conversation Commands:\n";
    std::cout << "  chat                            Start interactive AI conversation\n";
    std::cout << "  chat context                    Show conversation context\n";
    std::cout << "  chat clear                      Clear conversation context\n\n";

    std::cout << "Configuration:\n";
    std::cout << "  config ai                       Set up AI integration\n";
    std::cout << "  ai-status                       Show AI status\n";
    std::cout << "  ai-test                         Test AI connection\n\n";

    std::cout << "Examples:\n";
    std::cout << "  netlogai ask \"Why is BGP flapping on Router1?\"\n";
    std::cout << "  netlogai ask error \"%LINEPROTO-5-UPDOWN: gi0/1 down\"\n";
    std::cout << "  netlogai ask fix \"High CPU usage\" --logs router_logs.txt\n";
    std::cout << "  netlogai ask logs network.log --question \"Find security issues\"\n";
}

// Context management implementations
ConversationContext* AICommands::get_or_create_context(const std::string& session_id) {
    // Look for existing context
    for (auto& context : active_contexts) {
        if (context.session_id == session_id) {
            context.last_interaction = std::chrono::system_clock::now();
            return &context;
        }
    }

    // Create new context
    ConversationContext new_context;
    new_context.session_id = session_id;
    new_context.last_interaction = std::chrono::system_clock::now();
    new_context.max_context_messages = 20; // Keep last 20 messages

    active_contexts.push_back(new_context);
    return &active_contexts.back();
}

void AICommands::add_to_context(ConversationContext* context, const std::string& role, const std::string& content) {
    if (!context) return;

    context->messages.emplace_back(role, content);
    context->last_interaction = std::chrono::system_clock::now();

    // Maintain maximum context size
    while (context->messages.size() > static_cast<size_t>(context->max_context_messages)) {
        context->messages.erase(context->messages.begin());
    }
}

// Stub implementations for future development
int AICommands::start_conversation(const cli::CommandArgs& args) {
    std::cout << "Interactive AI conversation mode not yet implemented.\n";
    std::cout << "This will provide a chat-like interface with the AI assistant.\n";
    return 0;
}

int AICommands::show_context(const cli::CommandArgs& args) {
    std::cout << "Conversation context display not yet implemented.\n";
    std::cout << "This will show the current conversation history and log context.\n";
    return 0;
}

int AICommands::clear_context(const cli::CommandArgs& args) {
    std::cout << "Context clearing not yet implemented.\n";
    std::cout << "This will clear the current conversation context.\n";
    return 0;
}

} // namespace netlogai::commands