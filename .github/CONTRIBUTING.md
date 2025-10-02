# Contributing to NetLogAI

Thank you for your interest in contributing to NetLogAI! This document provides guidelines for contributing to the open source components of the NetLogAI ecosystem.

## 🏗️ Repository Structure

NetLogAI consists of multiple repositories:

- **[netlogai](https://github.com/leochong/netlogai)** (this repo) - Main platform and releases
- **[libnetlog](https://github.com/leochong/libnetlog)** - Core parsing library (MIT License)
- **[netlogai-plugin-sdk](https://github.com/leochong/netlogai-plugin-sdk)** - Plugin framework (MIT License)
- **[netlogai-parsers](https://github.com/leochong/netlogai-parsers)** - Community parsers (MIT License)
- **[netlogai-docs](https://github.com/leochong/netlogai-docs)** - Documentation (CC BY 4.0)

## 🤝 How to Contribute

### Reporting Bugs

1. **Check existing issues** to avoid duplicates
2. **Use the bug report template** when creating new issues
3. **Provide detailed information:**
   - NetLogAI version (`netlogai --version`)
   - Windows version and shell type
   - Steps to reproduce
   - Expected vs actual behavior
   - Log output or error messages

### Requesting Features

1. **Use the feature request template**
2. **Describe the use case** - what problem does it solve?
3. **Provide examples** of how it would work
4. **Consider alternatives** you've tried

### Requesting Parser Support

1. **Use the parser request template**
2. **Provide sample logs** (sanitized)
3. **Specify device details** (vendor, model, OS version)
4. **Indicate priority** for your use case

### Contributing to Open Source Components

#### Parser Development ([netlogai-parsers](https://github.com/leochong/netlogai-parsers))

1. Fork the `netlogai-parsers` repository
2. Create parser using NetLog Parser DSL (Lua-based)
3. Include sample logs and test cases
4. Submit pull request with:
   - Parser script (`device-name.nlp` or `.lua`)
   - Sample log files
   - Test cases
   - Documentation

#### Plugin Development ([netlogai-plugin-sdk](https://github.com/leochong/netlogai-plugin-sdk))

1. Fork the `netlogai-plugin-sdk` repository
2. Develop plugin using C++20 and SDK interfaces
3. Follow plugin API guidelines
4. Include tests and documentation
5. Submit pull request

#### Core Library ([libnetlog](https://github.com/leochong/libnetlog))

1. Fork the `libnetlog` repository
2. Make changes with proper tests
3. Ensure backwards compatibility
4. Follow C++20 best practices
5. Submit pull request with detailed description

#### Documentation ([netlogai-docs](https://github.com/leochong/netlogai-docs))

1. Fork the `netlogai-docs` repository
2. Improve guides, tutorials, or API docs
3. Use clear, concise language
4. Include examples and screenshots
5. Submit pull request

## 📝 Contribution Guidelines

### Code Style

**C++ Code:**
- Follow C++20 modern practices
- Use meaningful variable and function names
- Add comments for complex logic
- Include unit tests

**Parser Scripts (Lua/NetLog DSL):**
- Use descriptive pattern names
- Comment regex patterns
- Provide field extraction examples
- Include sample log lines

### Commit Messages

Use clear, descriptive commit messages:

```
Add Cisco Catalyst 9300 parser support

- Implement syslog pattern matching
- Extract interface, severity, and message fields
- Add test cases for common log patterns
- Include sample logs from IOS-XE 17.3
```

### Pull Request Process

1. **Create a focused PR** - one feature or fix per PR
2. **Write descriptive PR title and description**
3. **Include tests** for new functionality
4. **Update documentation** if needed
5. **Respond to review feedback** promptly

## 🔒 Security

**Do not submit security vulnerabilities as public issues.**

Instead, please report security issues privately:
- Email: (security contact TBD)
- Or create a private security advisory on GitHub

## 📄 Licensing

By contributing to NetLogAI open source components, you agree that your contributions will be licensed under the respective component's license:

- **libnetlog** - MIT License
- **netlogai-plugin-sdk** - MIT License
- **netlogai-parsers** - MIT License
- **netlogai-docs** - Creative Commons BY 4.0

The main NetLogAI platform (this repository) remains commercial software.

## 🙏 Recognition

Contributors to open source components will be recognized in:
- Repository README files
- Release notes
- Documentation acknowledgments

## 💬 Questions?

- **General questions:** [GitHub Discussions](https://github.com/leochong/netlogai/discussions)
- **Bug reports:** [GitHub Issues](https://github.com/leochong/netlogai/issues)
- **Documentation:** [NetLogAI Docs](https://github.com/leochong/netlogai-docs)

Thank you for contributing to NetLogAI! 🚀
