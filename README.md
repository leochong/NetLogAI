# NetLogAI - AI-Powered Network Log Analysis Platform

[![License: Commercial](https://img.shields.io/badge/License-Commercial-blue.svg)](LICENSE)
[![Windows](https://img.shields.io/badge/Platform-Windows-0078D6.svg)](https://github.com/leochong/netlogai/releases)
[![Version](https://img.shields.io/badge/Version-2.0.0-green.svg)](https://github.com/leochong/netlogai/releases/latest)
[![Downloads](https://img.shields.io/github/downloads/leochong/netlogai/total.svg)](https://github.com/leochong/netlogai/releases)

> **Transform network troubleshooting with AI-powered log analysis and an interactive command-line interface.**

NetLogAI is a professional network log analysis platform designed for network engineers who want powerful AI-assisted troubleshooting without the complexity of enterprise dashboards. Get instant insights into BGP flapping, interface issues, security threats, and performance problems through natural language queries.

---

## 🚀 Quick Start

### Download & Install

**Latest Release: v2.0.0** - [Download here](https://github.com/leochong/netlogai/releases/latest)

```bash
# 1. Download NetLogAI-v2.0.0-Windows-x64-portable-v2.zip
# 2. Extract to your preferred location
# 3. Run from command line:

netlogai --version
# Output: NetLogAI Core v2.0.0
```

### First Commands

```bash
# Analyze a log file
netlogai analyze router-logs.txt

# Ask AI about network issues
netlogai ask "Why is BGP flapping on R1?"

# Launch interactive shell
netlogai shell

# Check system status
netlogai status
```

---

## ✨ Key Features

### 🤖 AI-Powered Analysis
```bash
# Natural language queries for network engineers
netlogai ask "Why is BGP flapping?"
netlogai ask "Show me interface errors"
netlogai ask "Analyze high CPU on switches"

# Context-aware troubleshooting
netlogai analyze --pattern "interface_down"
netlogai correlate --timespan 1h
```

**Supported AI Providers:**
- Anthropic Claude (recommended)
- OpenAI GPT-4

### 🎯 Interactive Shell Experience

World-class terminal interface with professional features:

```bash
# Launch interactive shell
netlogai shell

# Interactive log viewing with vim-like controls
view logfile.txt               # Search, filter, bookmark logs
tail -f /var/log/syslog       # Live file monitoring

# File and device browsing
browse files                  # Interactive file browser with preview
browse devices                # Device management and selection

# Live log streaming
stream add router1 /logs/router1.log
stream start                  # Real-time multi-source monitoring

# Session management
session start "bgp-troubleshooting"
session replay session_123    # Replay previous workflows
```

**Shell Features:**
- ✅ Syntax highlighting for network logs
- ✅ Smart auto-completion with AI learning
- ✅ Command history and search (Ctrl+R)
- ✅ Session recording and replay
- ✅ Performance monitoring dashboard
- ✅ Real-time log streaming

### 📊 Git-Style CLI Commands

Familiar git-style interface for log management:

```bash
# Log viewing and filtering
netlogai log --oneline                 # Condensed log view
netlogai log --graph --device R1       # Device timeline
netlogai log --grep "BGP"             # Search logs
netlogai show HEAD~5                  # Show past entries

# Comparison and analysis
netlogai diff device1..device2        # Compare logs
netlogai blame interface gi0/1        # Interface-specific issues
netlogai timeline --interactive       # Interactive timeline view
```

### 📡 Direct Device Connectivity

Connect to network devices and stream logs in real-time:

```bash
# Add network devices
netlogai device add router1 --host 192.168.1.1 --type cisco-ios

# Fetch logs from devices
netlogai fetch router1 --output router1-logs.txt

# Device discovery
netlogai device discover              # Auto-discover network devices
netlogai device scan --subnet 192.168.1.0/24

# Real-time monitoring
netlogai device monitor router1       # Live log streaming
```

**Supported Vendors:**
- Cisco IOS / IOS-XE
- Cisco NX-OS
- Cisco ASA
- Juniper JunOS (planned)
- Arista EOS (planned)

### 🔌 Plugin System

Extensible architecture for custom analysis:

```bash
# Plugin management
netlogai plugin list                  # Available plugins
netlogai plugin install security_plugin.dll
netlogai plugin validate my-plugin.dll

# Security analysis plugins
netlogai security scan                # Run security analysis
netlogai security threats --severity high

# Performance plugins
netlogai perf baseline                # Establish baseline
netlogai perf compare --period 7d

# Network topology
netlogai topo discover                # Auto-discover topology
netlogai topo visualize               # Generate diagrams
```

---

## 📦 What Makes NetLogAI Different?

### ✅ **CLI-First, Not Dashboard-Heavy**
- Lightweight and fast - no heavy web interface
- Perfect for SSH sessions and automation
- Professional terminal experience

### ✅ **Conversational AI for Network Engineers**
- Ask questions in plain English: "Why is BGP flapping?"
- Domain-specific prompting for BGP, OSPF, interfaces
- Network engineer-focused responses

### ✅ **Direct Device Integration**
- SSH/Telnet to devices for live log streaming
- No need for external log collection infrastructure
- Real-time analysis as logs are generated

### ✅ **Advanced Interactive Features**
- Session recording and replay for workflow automation
- Vim-like log viewer with search and filtering
- Performance monitoring and optimization recommendations

---

## 🏗️ Open Source Ecosystem

NetLogAI is part of a multi-repository architecture supporting community contributions:

| Repository | License | Description |
|------------|---------|-------------|
| [**netlogai**](https://github.com/leochong/netlogai) | Commercial | Main platform and releases (this repo) |
| [**libnetlog**](https://github.com/leochong/libnetlog) | MIT | Core log parsing library |
| [**netlogai-plugin-sdk**](https://github.com/leochong/netlogai-plugin-sdk) | MIT | Plugin development framework |
| [**netlogai-parsers**](https://github.com/leochong/netlogai-parsers) | MIT | Community parser scripts |
| [**netlogai-docs**](https://github.com/leochong/netlogai-docs) | CC BY 4.0 | Documentation and guides |

**Community Contributions Welcome:**
- 🔧 Custom parsers for new device types
- 🔌 Plugins for specialized analysis
- 📚 Documentation improvements
- 🐛 Bug reports and feature requests

---

## 📋 System Requirements

**Windows 10/11 (x64)**
- Visual C++ 2022 Redistributable (auto-installed)
- 4GB RAM minimum (8GB recommended)
- 500MB disk space
- Internet connection for AI features (optional)

**Planned Support:**
- Linux (Ubuntu, RHEL, Debian)
- macOS

---

## 🎯 Installation & Setup

### 1. Download Release

Download the latest release from [Releases page](https://github.com/leochong/netlogai/releases/latest):
- `NetLogAI-v2.0.0-Windows-x64-portable-v2.zip` (1.4 MB)

### 2. Verify Package Integrity

```bash
# Verify SHA256 checksum
certutil -hashfile NetLogAI-v2.0.0-Windows-x64-portable-v2.zip SHA256

# Should match:
# c4ec1068f906c51ce2ec5b8f5fac43725a40f121273344adb91fd114e9ec69ba
```

### 3. Extract and Add to PATH

```bash
# Extract to preferred location (e.g., C:\Tools\NetLogAI)
# Add to PATH environment variable for global access
```

### 4. Configure AI Provider (Optional)

```bash
# Configure Anthropic Claude
netlogai config ai --provider anthropic --api-key sk-ant-xxxxx

# Or OpenAI
netlogai config ai --provider openai --api-key sk-xxxxx

# Check AI status
netlogai ai-status
```

### 5. Test Installation

```bash
# Check version
netlogai --version

# Run status check
netlogai status

# List available parsers
netlogai parser list

# Test AI (if configured)
netlogai ai-test
```

---

## 📚 Usage Examples

### Basic Log Analysis

```bash
# Analyze log file
netlogai analyze router-logs.txt

# Search for specific patterns
netlogai log --grep "OSPF" router-logs.txt

# Filter by severity
netlogai log --level ERROR,CRITICAL
```

### AI-Powered Queries

```bash
# Ask about specific issues
netlogai ask "Why is BGP neighbor 10.1.1.1 down?"

# Analyze error messages
netlogai ask error "%LINEPROTO-5-UPDOWN: gi0/1 down" --device-type cisco-ios

# Get troubleshooting suggestions
netlogai ask fix "High CPU usage on switches"
```

### Device Management

```bash
# Add devices
netlogai device add Router1 --host 192.168.1.1 --type cisco-ios --username admin

# List devices
netlogai device list

# Fetch logs
netlogai fetch Router1 --output logs/router1.txt

# Monitor live
netlogai device monitor Router1
```

### Interactive Shell

```bash
# Launch shell
netlogai shell

# Inside shell:
> view access-switch-01.log     # Interactive log viewer
> browse files                  # File browser
> stream add R1 /var/log/syslog # Live streaming
> session start "troubleshoot"  # Record session
> help                          # Show all commands
```

---

## 🛠️ Advanced Features

### Session Management

```bash
# Record troubleshooting workflows
netlogai session start "bgp-issue-2024-01"

# Run commands...
netlogai analyze router1.log
netlogai ask "Why is BGP flapping?"

# End session
netlogai session end

# Replay later
netlogai session list
netlogai session replay bgp-issue-2024-01
```

### Log Correlation

```bash
# Find correlated events across devices
netlogai correlate --timespan 1h --devices router1,switch1,firewall1

# Generate timeline
netlogai timeline --interactive --start "2024-01-01 14:00" --end "2024-01-01 15:00"
```

### Custom Parsers

```bash
# Install custom parser for specialized device
netlogai parser install my-custom-device.lua

# Test parser
netlogai parser test my-custom-device --sample sample.log

# Validate parser syntax
netlogai parser validate my-custom-device.lua
```

---

## 🔐 Security & Privacy

### Data Privacy
- **All log analysis happens locally** - No logs sent to external servers
- AI queries only send analyzed summaries, not raw logs
- API keys encrypted and stored securely
- No telemetry or usage tracking

### Security Features
- Plugin sandboxing with resource limits
- Input validation and sanitization
- Secure SSH/Telnet connections
- Configuration file encryption

### Package Verification
```bash
# Verify release integrity with SHA256
certutil -hashfile NetLogAI-v2.0.0-Windows-x64-portable-v2.zip SHA256

# Expected: c4ec1068f906c51ce2ec5b8f5fac43725a40f121273344adb91fd114e9ec69ba
```

**Note:** This release is currently unsigned. Windows may show security warnings - this is a false positive. Code signing certificate is in progress.

---

## 📖 Documentation

- **[Getting Started Guide](https://github.com/leochong/netlogai-docs)** - Complete setup and usage
- **[CLI Reference](https://github.com/leochong/netlogai-docs)** - All commands and options
- **[Interactive Shell Guide](https://github.com/leochong/netlogai-docs)** - Advanced shell features
- **[Plugin Development](https://github.com/leochong/netlogai-plugin-sdk)** - Build custom plugins
- **[Parser Scripting](https://github.com/leochong/netlogai-parsers)** - Write custom parsers
- **[API Reference](https://github.com/leochong/libnetlog)** - Library documentation

---

## 🤝 Community & Support

### Get Help
- **[GitHub Issues](https://github.com/leochong/netlogai/issues)** - Bug reports and feature requests
- **[GitHub Discussions](https://github.com/leochong/netlogai/discussions)** - Community Q&A
- **[Documentation](https://github.com/leochong/netlogai-docs)** - Comprehensive guides

### Contributing
We welcome contributions to open source components:
- 🔧 **Parser Library** - Add support for new devices
- 🔌 **Plugins** - Build custom analysis tools
- 📚 **Documentation** - Improve guides and tutorials
- 🌍 **Translations** - Multi-language support

See individual repository contribution guidelines.

---

## 🛣️ Roadmap

### v2.1.0 (Q1 2025)
- [ ] Real-time AI alerting and notifications
- [ ] Web-based dashboard (optional)
- [ ] Advanced correlation algorithms
- [ ] Multi-device orchestration
- [ ] Linux and macOS support

### v2.2.0 (Q2 2025)
- [ ] Cloud deployment options
- [ ] Enhanced plugin marketplace
- [ ] Machine learning anomaly detection
- [ ] Advanced visualization tools

### Community Goals
- [ ] 100+ community-contributed parsers
- [ ] 50+ third-party plugins
- [ ] Multi-language documentation
- [ ] Enterprise integrations (Splunk, ELK, etc.)

---

## 📄 License

NetLogAI Core is commercial software. See [LICENSE](LICENSE) for details.

Open source components:
- **libnetlog** - MIT License
- **netlogai-plugin-sdk** - MIT License
- **netlogai-parsers** - MIT License
- **netlogai-docs** - Creative Commons BY 4.0

---

## 💡 Why NetLogAI?

**Network engineers need fast answers, not complex dashboards.**

Traditional log analysis tools require:
- ❌ Complex setup and configuration
- ❌ Heavy web interfaces
- ❌ External log collection infrastructure
- ❌ Manual pattern matching and correlation

**NetLogAI provides:**
- ✅ Instant setup - download and run
- ✅ Lightweight CLI for SSH sessions
- ✅ Direct device connectivity
- ✅ AI-powered natural language queries
- ✅ Professional interactive shell

---

## 🙏 Acknowledgments

Special thanks to the network engineering community for feedback, testing, and contributions that made this release possible.

---

## 📞 Contact

- **Issues & Bugs:** [GitHub Issues](https://github.com/leochong/netlogai/issues)
- **Feature Requests:** [GitHub Discussions](https://github.com/leochong/netlogai/discussions)
- **Documentation:** [NetLogAI Docs](https://github.com/leochong/netlogai-docs)

---

**⭐ Star this repository if NetLogAI helps you troubleshoot faster!**

**📥 [Download v2.0.0](https://github.com/leochong/netlogai/releases/latest) and revolutionize your network troubleshooting!**
