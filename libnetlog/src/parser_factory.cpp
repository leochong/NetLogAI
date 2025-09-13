#include "libnetlog/parser_factory.hpp"
#include "libnetlog/parsers/cisco_ios_parser.hpp"
#include "libnetlog/parsers/cisco_nxos_parser.hpp"
#include "libnetlog/parsers/cisco_asa_parser.hpp"
#include "libnetlog/parsers/generic_syslog_parser.hpp"

namespace libnetlog {

ParserFactory& ParserFactory::instance() {
    static ParserFactory factory;
    return factory;
}

ParserFactory::ParserFactory() {
    register_builtin_parsers();
}

ParserFactory::ParserPtr ParserFactory::create_parser(DeviceType device_type) {
    auto it = parsers_.find(device_type);
    if (it != parsers_.end()) {
        return it->second();
    }
    return nullptr;
}

ParserFactory::ParserPtr ParserFactory::auto_detect_parser(const std::string& raw_message) {
    // Try each registered parser to see if it can handle the message
    std::vector<std::pair<DeviceType, int>> candidates;
    
    for (const auto& [device_type, creator] : parsers_) {
        auto parser = creator();
        if (parser && parser->can_parse(raw_message)) {
            // For now, use simple priority system
            // More specific parsers get higher priority
            int priority = 1;
            switch (device_type) {
                case DeviceType::CiscoIOS:
                case DeviceType::CiscoIOSXE:
                case DeviceType::CiscoNXOS:
                case DeviceType::CiscoASA:
                    priority = 3; // Cisco parsers are quite specific
                    break;
                case DeviceType::GenericSyslog:
                    priority = 1; // Generic parser is fallback
                    break;
                default:
                    priority = 2;
                    break;
            }
            candidates.emplace_back(device_type, priority);
        }
    }
    
    if (candidates.empty()) {
        return nullptr;
    }
    
    // Sort by priority (highest first)
    std::sort(candidates.begin(), candidates.end(), 
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Return the highest priority parser
    return create_parser(candidates[0].first);
}

bool ParserFactory::register_parser(DeviceType device_type, ParserCreator creator) {
    if (parsers_.find(device_type) != parsers_.end()) {
        return false; // Already registered
    }
    parsers_[device_type] = std::move(creator);
    return true;
}

bool ParserFactory::unregister_parser(DeviceType device_type) {
    auto it = parsers_.find(device_type);
    if (it != parsers_.end()) {
        parsers_.erase(it);
        return true;
    }
    return false;
}

std::vector<DeviceType> ParserFactory::get_supported_device_types() const {
    std::vector<DeviceType> types;
    types.reserve(parsers_.size());
    
    for (const auto& [device_type, creator] : parsers_) {
        types.push_back(device_type);
    }
    
    return types;
}

bool ParserFactory::is_supported(DeviceType device_type) const {
    return parsers_.find(device_type) != parsers_.end();
}

std::vector<ParserFactory::ParserInfo> ParserFactory::get_parser_info() const {
    std::vector<ParserInfo> info;
    info.reserve(parsers_.size());
    
    for (const auto& [device_type, creator] : parsers_) {
        auto parser = creator();
        if (parser) {
            ParserInfo parser_info;
            parser_info.name = parser->get_parser_name();
            parser_info.version = parser->get_version();
            parser_info.device_type = device_type;
            parser_info.supported_patterns = parser->get_supported_patterns();
            info.push_back(std::move(parser_info));
        }
    }
    
    return info;
}

void ParserFactory::register_builtin_parsers() {
    // Register Cisco IOS parser
    register_parser(DeviceType::CiscoIOS, []() -> ParserPtr {
        return std::make_unique<CiscoIOSParser>();
    });
    
    // Register Cisco IOS-XE parser (uses same parser as IOS for now)
    register_parser(DeviceType::CiscoIOSXE, []() -> ParserPtr {
        return std::make_unique<CiscoIOSParser>();
    });
    
    // Register Cisco NX-OS parser
    register_parser(DeviceType::CiscoNXOS, []() -> ParserPtr {
        return std::make_unique<CiscoNXOSParser>();
    });
    
    // Register Cisco ASA parser
    register_parser(DeviceType::CiscoASA, []() -> ParserPtr {
        return std::make_unique<CiscoASAParser>();
    });
    
    // Register generic syslog parser
    register_parser(DeviceType::GenericSyslog, []() -> ParserPtr {
        return std::make_unique<GenericSyslogParser>();
    });
}

} // namespace libnetlog