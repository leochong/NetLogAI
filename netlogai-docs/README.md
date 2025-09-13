# NetLogAI Documentation

**Comprehensive Documentation for the NetLogAI Ecosystem**

Complete guides, tutorials, and reference materials for network engineers using NetLogAI's log analysis platform.

[![Documentation Status](https://github.com/NetLogAI/netlogai-docs/workflows/Build/badge.svg)](https://github.com/NetLogAI/netlogai-docs/actions)
[![License: CC BY-SA 4.0](https://img.shields.io/badge/License-CC%20BY--SA%204.0-lightgrey.svg)](https://creativecommons.org/licenses/by-sa/4.0/)
[![Website](https://img.shields.io/badge/Website-docs.netlogai.com-blue.svg)](https://docs.netlogai.com)

## 📚 Documentation Structure

### 🚀 Getting Started
- [Quick Start Guide](user-guide/getting-started.md) - Get up and running in 15 minutes
- [Installation Guide](user-guide/installation.md) - Detailed installation instructions
- [Your First Analysis](user-guide/first-analysis.md) - Step-by-step tutorial
- [Configuration Guide](user-guide/configuration.md) - System and AI setup

### 👤 User Guide
- [Command Reference](user-guide/commands/) - Complete CLI command documentation
- [AI Integration Setup](user-guide/ai-setup.md) - Configure Anthropic Claude and OpenAI
- [Log Management](user-guide/log-management.md) - Importing and organizing logs
- [Analysis Workflows](user-guide/workflows/) - Common analysis patterns
- [Troubleshooting Guide](user-guide/troubleshooting.md) - Solutions to common issues

### 🧑‍💻 Developer Guide
- [Plugin Development](developer-guide/plugin-development.md) - Create custom plugins
- [Parser Creation](developer-guide/parser-development.md) - Build device parsers
- [API Reference](developer-guide/api/) - Complete API documentation
- [Contributing](developer-guide/contributing.md) - Join the development community
- [Architecture Overview](developer-guide/architecture.md) - System design and internals

### 🏢 Enterprise Guide
- [Deployment Guide](enterprise-guide/deployment.md) - Large-scale deployments
- [Security Best Practices](enterprise-guide/security.md) - Secure configuration
- [Integration Guide](enterprise-guide/integrations.md) - SIEM and monitoring systems
- [Compliance](enterprise-guide/compliance.md) - Regulatory compliance guide
- [Professional Support](enterprise-guide/support.md) - Enterprise support options

### 📝 Tutorials
- [Network Troubleshooting](tutorials/network-troubleshooting/) - Real-world scenarios
- [Security Analysis](tutorials/security-analysis/) - Threat detection workflows
- [Performance Monitoring](tutorials/performance-monitoring/) - Network performance analysis
- [Automation Scripts](tutorials/automation/) - Scripting and automation examples

## 🎯 Quick Navigation

### New Users Start Here
1. **[Installation Guide](user-guide/installation.md)** - Set up NetLogAI on your system
2. **[AI Provider Setup](user-guide/ai-setup.md)** - Configure AI integration
3. **[First Analysis Tutorial](user-guide/first-analysis.md)** - Analyze your first logs
4. **[Command Cheat Sheet](user-guide/commands/cheat-sheet.md)** - Essential commands

### For Network Engineers
- **[BGP Troubleshooting](tutorials/network-troubleshooting/bgp-analysis.md)** - Diagnose BGP issues
- **[Interface Flapping Analysis](tutorials/network-troubleshooting/interface-flapping.md)** - Identify root causes
- **[Security Incident Response](tutorials/security-analysis/incident-response.md)** - Respond to threats
- **[Performance Baselining](tutorials/performance-monitoring/baseline-creation.md)** - Establish baselines

### For Developers
- **[Your First Plugin](developer-guide/plugin-tutorial.md)** - Create a simple plugin
- **[Parser Development](developer-guide/parser-tutorial.md)** - Write device parsers
- **[API Integration](developer-guide/api-integration.md)** - Integrate with external systems
- **[Testing Framework](developer-guide/testing.md)** - Test plugins and parsers

### For IT Managers
- **[ROI Calculator](enterprise-guide/roi-calculator.md)** - Calculate NetLogAI value
- **[Deployment Planning](enterprise-guide/deployment-planning.md)** - Plan your rollout
- **[Training Resources](enterprise-guide/training.md)** - Team training materials
- **[Success Metrics](enterprise-guide/success-metrics.md)** - Measure success

## 🔍 Popular Topics

### Most Viewed Documentation
1. [Cisco IOS Log Analysis](tutorials/network-troubleshooting/cisco-ios-analysis.md)
2. [Setting up Anthropic Claude](user-guide/ai-setup.md#anthropic-claude)
3. [Creating Security Plugins](developer-guide/security-plugin-tutorial.md)
4. [Command Line Reference](user-guide/commands/core-commands.md)
5. [Performance Tuning](user-guide/performance-tuning.md)

### Recently Updated
- [AI Integration Guide](user-guide/ai-setup.md) - Updated for Claude 3.5 Sonnet
- [Plugin SDK Documentation](developer-guide/plugin-sdk/) - New security features
- [Enterprise Deployment](enterprise-guide/deployment.md) - Kubernetes support
- [API Documentation](developer-guide/api/) - REST API v2.0

## 🛠️ Contributing to Documentation

We welcome contributions to improve our documentation! Whether you're fixing typos, adding examples, or writing new guides, your help makes NetLogAI better for everyone.

### How to Contribute
1. **Fork this repository**
2. **Create a feature branch**: `git checkout -b improve-bgp-docs`
3. **Make your changes** using Markdown
4. **Test your changes** locally
5. **Submit a pull request**

### Documentation Guidelines
- **Clear and Concise**: Write for network engineers of all skill levels
- **Real Examples**: Include actual log samples and command outputs
- **Step-by-Step**: Break complex procedures into clear steps
- **Screenshots**: Add screenshots for UI-based procedures
- **Links**: Link to related documentation sections

### What We Need
- **More Tutorials**: Real-world troubleshooting scenarios
- **Device Coverage**: Documentation for additional network devices
- **Use Cases**: Industry-specific use cases and workflows
- **Translations**: Documentation in other languages
- **Video Content**: Screencasts and demonstration videos

## 📖 Documentation Format

### Markdown Standards
- Use standard Markdown with GitHub Flavored Markdown extensions
- Include code blocks with syntax highlighting
- Add table of contents for long documents
- Use consistent heading hierarchy

### Code Examples
```bash
# Always include command examples
nla log --device 192.168.1.1 --grep "BGP"

# Show expected output
Dec 15 10:30:15: %BGP-5-ADJCHANGE: neighbor 10.0.0.2 Down - BGP session terminated
```

### File Organization
```
docs/
├── user-guide/          # End-user documentation
├── developer-guide/     # Developer documentation  
├── enterprise-guide/    # Enterprise deployment
├── tutorials/           # Step-by-step tutorials
├── reference/           # Command and API reference
└── assets/              # Images, videos, downloads
```

## 🌐 Multilingual Support

### Available Languages
- **English** 🇺🇸 - Complete documentation
- **Spanish** 🇪🇸 - Core guides translated
- **German** 🇩🇪 - Getting started guide
- **Japanese** 🇯🇵 - User guide (partial)
- **French** 🇫🇷 - Installation guide

### Translation Guidelines
- Maintain technical accuracy
- Keep code examples in English
- Translate UI elements consistently
- Include cultural context where needed

## 📊 Documentation Analytics

### Usage Statistics (Last 30 Days)
- **Page Views**: 45,000+ across all documentation
- **Top Pages**: Getting Started (8,500), AI Setup (6,200), Commands (5,800)
- **User Feedback**: 4.8/5.0 average rating
- **Search Queries**: "BGP analysis", "AI setup", "plugin development"

### User Feedback
> "The step-by-step tutorials saved me hours of troubleshooting. The BGP analysis guide is particularly excellent." - Senior Network Engineer

> "Clear documentation with practical examples. The plugin development guide helped me create my first custom analyzer." - Network Automation Engineer

## 🔧 Local Development

### Building Documentation Locally

```bash
# Clone the repository
git clone https://github.com/NetLogAI/netlogai-docs.git
cd netlogai-docs

# Install dependencies
npm install

# Start development server
npm run dev

# Build static site
npm run build

# Deploy to GitHub Pages
npm run deploy
```

### Documentation Tools
- **Generator**: [VitePress](https://vitepress.dev/) for fast static site generation
- **Markdown**: GitHub Flavored Markdown with extensions
- **Search**: Full-text search with Algolia
- **Analytics**: Google Analytics for usage tracking
- **Comments**: Giscus for community discussions

## 📞 Getting Help

### Documentation Issues
- **Bug Reports**: [GitHub Issues](https://github.com/NetLogAI/netlogai-docs/issues)
- **Improvements**: [GitHub Discussions](https://github.com/NetLogAI/netlogai-docs/discussions)
- **Live Chat**: [Discord Community](https://discord.gg/netlogai)

### Content Requests
- **Missing Topics**: Request new documentation topics
- **Outdated Content**: Report outdated information
- **Clarity Issues**: Suggest improvements for unclear sections

## 📄 License

This documentation is licensed under the [Creative Commons Attribution-ShareAlike 4.0 International License](https://creativecommons.org/licenses/by-sa/4.0/).

You are free to:
- **Share** — copy and redistribute the material in any medium or format
- **Adapt** — remix, transform, and build upon the material

Under the following terms:
- **Attribution** — Give appropriate credit to NetLogAI
- **ShareAlike** — Distribute under the same license
- **No Additional Restrictions** — No additional legal or technical restrictions

## 🏆 Recognition

### Documentation Awards
- **2024 Developer Documentation Excellence Award** - DevDocs Awards
- **Best Technical Documentation** - Network Engineering Community
- **Open Source Documentation of the Year** - FOSS Awards

### Contributors Hall of Fame
- **@doc-master**: 150+ documentation contributions
- **@tutorial-queen**: 25 comprehensive tutorials
- **@api-expert**: Complete API documentation overhaul
- **@translation-team**: Multi-language support implementation

---

**Empowering network engineers with knowledge and expertise! 📖🚀**

Visit our complete documentation at [docs.netlogai.com](https://docs.netlogai.com) or join our [Discord community](https://discord.gg/netlogai) for live support and discussions.