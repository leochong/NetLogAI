# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**NetLogAI** is a network log analysis CLI tool with dual licensing strategy:
- **Technology Stack:** C++20 with MSVC 2022, CMake build system, vcpkg dependency management, Lua scripting engine
- **Primary IDE:** Visual Studio Code with CMake Tools extension
- **Platform:** Windows-first development with cross-platform potential

## Multi-Repository Architecture

This is part of a 5-repository architecture under the NetLogAI GitHub organization:

1. **netlogai-core** (PRIVATE - Commercial License) - Advanced shell, AI integration, enterprise analytics
2. **libnetlog** (PUBLIC - MIT License) - Core log parsing library with Cisco IOS/IOS-XE, NX-OS, and ASA parsers
3. **netlogai-plugin-sdk** (PUBLIC - MIT License) - Plugin development framework and interfaces
4. **netlogai-parsers** (PUBLIC - MIT License) - Community-contributed parser scripts
5. **netlogai-docs** (PUBLIC - Creative Commons) - Comprehensive documentation

## Core Commands Structure

The CLI follows a git-style command structure with these main categories:

### Log Management Commands
```bash
# Git-style commands for log management
nla log --online                    # Show condensed log view
nla log --graph --device R1         # Show device-specific timeline
nla log --grep "BGP"               # Search for BGP-related entries
nla show HEAD~5                    # Show logs from 5 entries ago
nla diff device1..device2          # Compare logs between devices
nla blame interface gi0/1          # Show interface-specific issues

# Log fetching and management
nla fetch --device 192.168.1.1     # Fetch logs from device
nla pull --all                     # Update all device logs
nla status                         # Show log fetch status
nla branch --device-group datacenter # Work with device groups
```

### AI-Powered Analysis
```bash
# AI-powered queries
nla ask "Why is BGP flapping on R1?"
nla analyze --pattern "interface down"
nla suggest --problem "high CPU"
nla chat "Explain this error: %LINEPROTO-5-UPDOWN"
```

### Advanced Analysis
```bash
# Advanced analysis commands
nla correlate --timespan 1h         # Find correlated events
nla timeline --interactive          # Interactive timeline view
nla export --format json           # Export filtered logs
```

### Parser Management
```bash
# Custom parser management
nla parser list                     # List available parsers
nla parser install custom-device.lua # Install custom parser
nla parser test my-parser          # Test parser against sample logs
nla parser validate script.nlp     # Validate parser script
```

### Plugin Commands
```bash
# Security plugin
nla security scan                   # Run security analysis
nla security threats --severity high

# Performance plugin
nla perf baseline                   # Establish performance baseline
nla perf compare --period 7d

# Network topology plugin
nla topo discover                   # Auto-discover topology
nla topo visualize                  # Generate topology diagram
```

## Development Environment Setup

### Required Scripts
- `setup-dev-env.bat` - Development environment setup automation
- `build-release.bat` - Windows release build script
- `run-tests.bat` - Test execution script
- `update-submodules.bat` - Submodule update scripts

### Build Commands
```bash
# CMake build system for mixed licensing
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release

# Separate builds for open/closed source components
cmake --build build --target libnetlog        # Open source library
cmake --build build --target netlogai-core    # Commercial core
```

### Testing
```bash
# Run comprehensive test suite
./run-tests.bat

# Test specific components
ctest --test-dir build -R "libnetlog_tests"   # Parser library tests
ctest --test-dir build -R "plugin_tests"      # Plugin framework tests
```

## Project Structure Highlights

### Core Architecture
- **src/core/** - CLOSED SOURCE (Commercial License) - Shell interface, AI integration, advanced analysis
- **src/libciscolog/** - OPEN SOURCE (MIT License) - Log parsing library and device-specific parsers
- **src/plugin-sdk/** - OPEN SOURCE (MIT License) - Plugin development framework
- **examples/plugins/** - Security, performance, and topology example plugins

### Key Components
- **Lua Scripting Engine** - NetLog Parser DSL (Domain-Specific Language) interpreter
- **AI Integration Framework** - Abstract provider interface supporting Anthropic Claude and OpenAI GPT
- **Plugin System** - Extensible architecture with security sandbox
- **Mixed Licensing Build System** - CMake configuration for proper open/closed source separation

### Configuration
- **AI Provider Setup** - Users configure through interactive wizard or config files
- **Environment Variables** - `NETLOGAI_AI_PROVIDER`, `NETLOGAI_ANTHROPIC_KEY` for CI/CD
- **Device Profiles** - JSON configuration for network device connections

## Security Considerations

### .gitignore Critical Patterns
- API keys and secrets (`**/api-keys.json`, `**/secrets.json`, `**/.env`)
- Commercial license validation files (`license-validation/`, `activation/`)
- Production configuration (`**/config/production.json`)
- Build artifacts and third-party downloads

### Development Guidelines
- Never commit API keys or commercial secrets
- Use proper licensing boundaries in build system
- Implement security sandbox for third-party plugins
- Follow Windows-specific security practices

## AI Integration Architecture

The system supports multiple AI providers through an abstract interface:

```cpp
// Core AI configuration system
namespace NetLogAI::AI {
    enum class AIProvider { Anthropic, OpenAI, None };
    
    class AIConfigurationManager {
        // API key management and encryption
        // Provider switching and validation
    };
}
```

Users configure AI providers via:
1. Interactive setup wizard: `cla setup ai --provider anthropic --api-key sk-ant-xxxx`
2. Configuration file: `config/ai-config.json`
3. Environment variables for CI/CD

This repository represents the foundation phase (Weeks 1-4) focusing on repository setup, scripted parser engine development, and core infrastructure with VS Code/MSVC toolchain integration.
- Add to rememory. This Github repo plan and recommendation. I will continue this plan tomorrow
- remember how to checkin to this GitHub repo
- remember these strong points about this project:  What Makes NetLogAI Different (But Not Necessarily "World First"):

  ✅ Conversational interface specifically for network logs - Most tools use    
   dashboards, not chat
  ✅ Direct integration with network devices + AI analysis - Combined device    
   management with AI
  ✅ Network engineer-focused prompting - Specialized for BGP, OSPF,
  interface troubleshooting
  ✅ Lightweight CLI tool - Not a heavy enterprise platform
- save my progress
- go ahead and remember tis GNS3 strategy