# libnetlog

**Open Source Network Log Parsing Library**

A high-performance C++ library for parsing and analyzing network device logs. Built for network engineers, by network engineers.

[![Build Status](https://github.com/NetLogAI/libnetlog/workflows/CI/badge.svg)](https://github.com/NetLogAI/libnetlog/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)

## 🎯 Overview

libnetlog provides a robust, extensible foundation for parsing network device logs from major vendors. Whether you're building monitoring tools, troubleshooting automation, or log analysis applications, libnetlog handles the complexity of vendor-specific log formats.

### Supported Platforms
- **Cisco IOS/IOS-XE**: Routers, switches, and security appliances
- **Cisco NX-OS**: Nexus data center switches
- **Cisco ASA**: Adaptive Security Appliances
- **Generic Syslog**: RFC3164/RFC5424 compliant devices

## ✨ Key Features

### High-Performance Parsing
- **Zero-Copy Design**: Minimize memory allocations during parsing
- **Streaming Support**: Process logs in real-time as they arrive
- **Multi-Threading**: Parallel processing of log streams
- **Memory Efficient**: Handle multi-GB log files with minimal RAM

### Intelligent Log Structure
- **Structured Output**: Convert raw logs to structured JSON/objects
- **Timestamp Normalization**: Handle various timestamp formats consistently
- **Severity Mapping**: Standardize severity levels across vendors
- **Message Classification**: Categorize log messages by type and importance

### Extensible Architecture
- **Plugin System**: Add support for new device types
- **Custom Patterns**: Define regex patterns for proprietary formats
- **Field Extraction**: Flexible field mapping and data extraction
- **Validation Rules**: Built-in data validation and error handling

## 🚀 Quick Start

### Installation

#### Using vcpkg (Recommended)
```bash
vcpkg install libnetlog
```

#### Building from Source
```bash
git clone https://github.com/NetLogAI/libnetlog.git
cd libnetlog
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel
```

### Basic Usage

```cpp
#include <libnetlog/parsers.hpp>
#include <libnetlog/log_entry.hpp>

int main() {
    // Create parser for Cisco IOS logs
    auto parser = libnetlog::createParser(libnetlog::DeviceType::CiscoIOS);
    
    // Parse a log line
    std::string logLine = "Dec 15 10:30:15.123: %LINEPROTO-5-UPDOWN: Line protocol on Interface GigabitEthernet0/1, changed state to down";
    
    auto entry = parser->parse(logLine);
    if (entry) {
        std::cout << "Timestamp: " << entry->timestamp << std::endl;
        std::cout << "Severity: " << entry->severity << std::endl;
        std::cout << "Facility: " << entry->facility << std::endl;
        std::cout << "Message: " << entry->message << std::endl;
        std::cout << "Interface: " << entry->getField("interface") << std::endl;
    }
    
    return 0;
}
```

### Stream Processing
```cpp
#include <libnetlog/stream_parser.hpp>

// Process logs in real-time
libnetlog::StreamParser stream_parser(libnetlog::DeviceType::CiscoIOS);

stream_parser.onLogEntry([](const libnetlog::LogEntry& entry) {
    if (entry.severity >= libnetlog::Severity::Warning) {
        std::cout << "Warning: " << entry.message << std::endl;
    }
});

// Feed log data to the parser
stream_parser.processData(log_data);
```

## 🔧 Supported Log Formats

### Cisco IOS/IOS-XE
```
Dec 15 10:30:15.123: %LINEPROTO-5-UPDOWN: Line protocol on Interface GigabitEthernet0/1, changed state to down
*Mar 1 00:00:01.000: %SYS-5-CONFIG_I: Configured from console by admin on vty0
```

### Cisco NX-OS
```
2023 Dec 15 10:30:15 switch-01 %ETHPORT-5-IF_DOWN_INTERFACE_REMOVED: Interface Ethernet1/1 is down
2023 Dec 15 10:30:15 switch-01 %VSHD-5-VSHD_SYSLOG_CONFIG_I: Configured from vty by admin on console0
```

### Cisco ASA
```
Dec 15 2023 10:30:15: %ASA-6-302013: Built inbound TCP connection 12345 for outside:192.168.1.100/80
Dec 15 2023 10:30:15: %ASA-4-106023: Deny tcp src outside:192.168.1.200/443 dst inside:10.0.0.1/80
```

## 📚 Advanced Features

### Custom Field Extraction
```cpp
// Extract interface information from log messages
auto parser = libnetlog::createParser(libnetlog::DeviceType::CiscoIOS);
parser->addFieldExtractor("interface", R"(Interface\s+(\S+))");
parser->addFieldExtractor("vlan", R"(VLAN\s+(\d+))");

auto entry = parser->parse(log_line);
std::string interface = entry->getField("interface");
std::string vlan = entry->getField("vlan");
```

### Filtering and Processing
```cpp
// Set up filters for specific log types
libnetlog::LogFilter filter;
filter.setSeverityMin(libnetlog::Severity::Warning);
filter.addFacilityFilter("LINEPROTO");
filter.addPatternFilter(R"(Interface.*down)");

auto filtered_entries = parser->parseWithFilter(log_lines, filter);
```

### Statistics and Metrics
```cpp
// Get parsing statistics
auto stats = parser->getStatistics();
std::cout << "Total entries parsed: " << stats.total_entries << std::endl;
std::cout << "Parse errors: " << stats.parse_errors << std::endl;
std::cout << "Processing rate: " << stats.entries_per_second << " entries/sec" << std::endl;
```

## 🧪 Testing

```bash
# Run unit tests
ctest --test-dir build

# Run with sample log files
./build/tests/test_parser --log-file samples/cisco_ios.log

# Performance benchmarks
./build/benchmarks/parser_benchmark
```

## 📖 Documentation

- [API Reference](docs/api/README.md)
- [Parser Development Guide](docs/parsers.md)
- [Performance Tuning](docs/performance.md)
- [Contributing Guidelines](CONTRIBUTING.md)

## 🤝 Contributing

We welcome contributions from the network engineering community! Whether you're adding support for new devices, improving parsing accuracy, or optimizing performance, your contributions help everyone.

### Ways to Contribute
- **Device Support**: Add parsers for new network devices
- **Pattern Improvement**: Enhance regex patterns for better accuracy
- **Performance**: Optimize parsing algorithms
- **Documentation**: Improve guides and examples
- **Bug Reports**: Report parsing issues with sample logs

### Getting Started
1. Fork the repository
2. Create a feature branch: `git checkout -b feature/new-device-support`
3. Add your changes with tests
4. Submit a pull request

## 📊 Performance Benchmarks

| Device Type | Parsing Rate | Memory Usage | CPU Usage |
|-------------|--------------|-------------|-----------|
| Cisco IOS   | 50K lines/sec | 50MB/1M lines | ~15% |
| Cisco NX-OS | 45K lines/sec | 48MB/1M lines | ~12% |
| Cisco ASA   | 60K lines/sec | 55MB/1M lines | ~18% |
| Generic Syslog | 80K lines/sec | 40MB/1M lines | ~10% |

*Benchmarks on Intel i7-10700K, single-threaded*

## 🔒 Security

libnetlog is designed with security in mind:
- **Input Validation**: All log input is validated to prevent injection attacks
- **Memory Safety**: Modern C++ practices prevent buffer overflows
- **No External Dependencies**: Minimal attack surface
- **Regular Security Audits**: Automated security scanning in CI/CD

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🌟 Used By

libnetlog powers log analysis in:
- Network monitoring platforms
- Security information systems
- Automated troubleshooting tools
- Research and academic projects

---

**Built with ❤️ by the network engineering community**

For support and discussions, join our [GitHub Discussions](https://github.com/NetLogAI/libnetlog/discussions) or reach out on our [Discord server](https://discord.gg/netlogai).