#include "libnetlog/utils/regex_patterns.hpp"

namespace libnetlog {
namespace utils {

// Timestamp patterns
const std::regex RegexPatterns::ISO8601_TIMESTAMP(R"(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}(?:\.\d+)?(?:Z|[+-]\d{2}:\d{2})?)");
const std::regex RegexPatterns::SYSLOG_TIMESTAMP(R"(\w{3}\s+\d{1,2}\s+\d{2}:\d{2}:\d{2})");
const std::regex RegexPatterns::CISCO_TIMESTAMP(R"(\*?\w{3}\s+\d{1,2}\s+\d{2}:\d{2}:\d{2}(?:\.\d+)?)");
const std::regex RegexPatterns::SIMPLE_TIME(R"(\d{2}:\d{2}:\d{2}(?:\.\d+)?)");

// Network patterns
const std::regex RegexPatterns::IPV4_ADDRESS(R"(\b(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\b)");
const std::regex RegexPatterns::IPV6_ADDRESS(R"(\b(?:[0-9a-fA-F]{1,4}:){7}[0-9a-fA-F]{1,4}\b|\b::1\b|\b::\b|(?:[0-9a-fA-F]{1,4}:)*::[0-9a-fA-F]{1,4}(?::[0-9a-fA-F]{1,4})*)");
const std::regex RegexPatterns::MAC_ADDRESS(R"(\b[0-9a-fA-F]{2}[:-][0-9a-fA-F]{2}[:-][0-9a-fA-F]{2}[:-][0-9a-fA-F]{2}[:-][0-9a-fA-F]{2}[:-][0-9a-fA-F]{2}\b)");
const std::regex RegexPatterns::INTERFACE_NAME(R"(\b(?:Gi|Fa|Et|Te|Se|Lo|Vl|Po|Tun|Tu|Mgmt)\d+(?:/\d+)*(?:\.\d+)?\b)");

// Syslog patterns
const std::regex RegexPatterns::SYSLOG_PRIORITY(R"(<(\d+)>)");
const std::regex RegexPatterns::SYSLOG_RFC3164(R"(<\d+>\w{3}\s+\d{1,2}\s+\d{2}:\d{2}:\d{2}\s+\S+\s+.+)");
const std::regex RegexPatterns::SYSLOG_RFC5424(R"(<\d+>\d+\s+\S+\s+\S+\s+\S+\s+\S+\s+\S+\s+\S*\s*.*)");

// Cisco specific patterns
const std::regex RegexPatterns::CISCO_MESSAGE_ID(R"(%([A-Z_]+)-(\d+)-([A-Z_]+))");
const std::regex RegexPatterns::CISCO_FACILITY(R"(%([A-Z_]+)-)");

// Common log elements
const std::regex RegexPatterns::HOSTNAME(R"(\b[a-zA-Z0-9]([a-zA-Z0-9\-]{0,61}[a-zA-Z0-9])?(\.[a-zA-Z0-9]([a-zA-Z0-9\-]{0,61}[a-zA-Z0-9])?)*\b)");
const std::regex RegexPatterns::PROCESS_NAME(R"(\b[a-zA-Z][a-zA-Z0-9_-]*\b)");
const std::regex RegexPatterns::SEVERITY_WORD(R"(\b(?:emerg|alert|crit|err|warn|notice|info|debug)\b)");

bool RegexPatterns::is_ipv4(const std::string& str) {
    return std::regex_match(str, IPV4_ADDRESS);
}

bool RegexPatterns::is_ipv6(const std::string& str) {
    return std::regex_match(str, IPV6_ADDRESS);
}

bool RegexPatterns::is_mac_address(const std::string& str) {
    return std::regex_match(str, MAC_ADDRESS);
}

std::vector<std::string> RegexPatterns::extract_ipv4_addresses(const std::string& str) {
    std::vector<std::string> addresses;
    std::sregex_iterator begin(str.begin(), str.end(), IPV4_ADDRESS);
    std::sregex_iterator end;
    
    for (std::sregex_iterator i = begin; i != end; ++i) {
        addresses.push_back(i->str());
    }
    
    return addresses;
}

std::vector<std::string> RegexPatterns::extract_ipv6_addresses(const std::string& str) {
    std::vector<std::string> addresses;
    std::sregex_iterator begin(str.begin(), str.end(), IPV6_ADDRESS);
    std::sregex_iterator end;
    
    for (std::sregex_iterator i = begin; i != end; ++i) {
        addresses.push_back(i->str());
    }
    
    return addresses;
}

std::vector<std::string> RegexPatterns::extract_interface_names(const std::string& str) {
    std::vector<std::string> interfaces;
    std::sregex_iterator begin(str.begin(), str.end(), INTERFACE_NAME);
    std::sregex_iterator end;
    
    for (std::sregex_iterator i = begin; i != end; ++i) {
        interfaces.push_back(i->str());
    }
    
    return interfaces;
}

} // namespace utils
} // namespace libnetlog