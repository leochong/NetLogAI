# NetLogAI Multi-Repository Deployment Guide

This guide walks you through setting up the complete NetLogAI multi-repository architecture on GitHub.

## 📋 Prerequisites

- GitHub organization owner permissions
- Git command line tools
- GitHub CLI (`gh`) for repository creation
- Windows development environment (for testing)

## 🏗️ Step 1: Create GitHub Organization

### 1.1 Organization Setup
```bash
# Create organization on GitHub web interface
# Organization name: NetLogAI
# Organization email: admin@netlogai.com
# Organization type: Company
```

### 1.2 Organization Configuration
- **Profile**: Add logo, description, and website
- **Member Privileges**: Set default permissions
- **Security**: Enable 2FA requirement
- **Billing**: Set up billing if using private repositories

## 📦 Step 2: Create Repositories

### 2.1 Private Repository (netlogai-core)
```bash
# Create private repository
gh repo create NetLogAI/netlogai-core --private --description "NetLogAI Core - Enterprise Network Log Analysis Platform"

# Clone and set up
git clone https://github.com/NetLogAI/netlogai-core.git
cd netlogai-core

# Copy files from this setup
cp -r ../NetLogAI/netlogai-core/* .

# Initial commit
git add .
git commit -m "feat: initial repository setup with core architecture

- Enterprise CLI application structure
- Commercial license and professional README  
- CMake build system with vcpkg integration
- AI integration framework (Anthropic Claude, OpenAI GPT)
- Plugin system architecture
- Security-focused .gitignore with API key protection
- GitHub Actions CI/CD for Windows, security scanning
- Development environment setup automation

🤖 Generated with NetLogAI repository setup"

git push origin main
```

### 2.2 Public Repository - libnetlog (MIT License)
```bash
# Create public repository
gh repo create NetLogAI/libnetlog --public --description "Open Source Network Log Parsing Library"

git clone https://github.com/NetLogAI/libnetlog.git
cd libnetlog

# Copy files
cp -r ../NetLogAI/libnetlog/* .

# Initial commit
git add .
git commit -m "feat: initial libnetlog library setup

- High-performance C++20 log parsing library
- Support for Cisco IOS/IOS-XE/NX-OS/ASA devices  
- Extensible parser architecture
- Comprehensive CMake build system
- Cross-platform CI/CD (Windows, Linux, macOS)
- Performance benchmarking and testing framework

🤖 Generated with NetLogAI repository setup"

git push origin main
```

### 2.3 Public Repository - netlogai-plugin-sdk (MIT License)
```bash
# Create public repository  
gh repo create NetLogAI/netlogai-plugin-sdk --public --description "NetLogAI Plugin Development Framework"

git clone https://github.com/NetLogAI/netlogai-plugin-sdk.git
cd netlogai-plugin-sdk

# Copy files
cp -r ../NetLogAI/netlogai-plugin-sdk/* .

# Initial commit
git add .
git commit -m "feat: initial plugin SDK setup

- Extensible plugin development framework
- Type-safe C++20 interfaces and security sandbox
- Plugin templates and development tools  
- Comprehensive testing framework
- Hot reload and dependency management
- Cross-platform support and documentation

🤖 Generated with NetLogAI repository setup"

git push origin main
```

### 2.4 Public Repository - netlogai-parsers (MIT License)
```bash
# Create public repository
gh repo create NetLogAI/netlogai-parsers --public --description "Community-Contributed Network Device Parsers"

git clone https://github.com/NetLogAI/netlogai-parsers.git
cd netlogai-parsers

# Copy files
cp -r ../NetLogAI/netlogai-parsers/* .

# Initial commit
git add .
git commit -m "feat: initial parser collection setup

- Community-driven Lua parser collection
- Support for major network device vendors
- Automated testing and validation framework
- Parser development tools and templates
- Performance benchmarking and security scanning
- Comprehensive documentation and examples

🤖 Generated with NetLogAI repository setup"

git push origin main
```

### 2.5 Public Repository - netlogai-docs (Creative Commons)
```bash
# Create public repository
gh repo create NetLogAI/netlogai-docs --public --description "Comprehensive Documentation for NetLogAI Ecosystem"

git clone https://github.com/NetLogAI/netlogai-docs.git
cd netlogai-docs

# Copy files
cp -r ../NetLogAI/netlogai-docs/* .

# Initial commit  
git add .
git commit -m "feat: initial documentation setup

- Comprehensive user and developer guides
- Multi-language support and accessibility
- VitePress static site generation
- SEO optimization and analytics
- Automated deployment to GitHub Pages
- Community contribution guidelines

🤖 Generated with NetLogAI repository setup"

git push origin main
```

## 🔗 Step 3: Set Up Submodules

### 3.1 Initialize Submodules in Core Repository
```bash
cd netlogai-core

# Add submodules
git submodule add https://github.com/NetLogAI/libnetlog.git third-party/libnetlog
git submodule add https://github.com/NetLogAI/netlogai-plugin-sdk.git third-party/plugin-sdk  
git submodule add https://github.com/NetLogAI/netlogai-parsers.git third-party/parsers

# Initialize and update
git submodule init
git submodule update --recursive

# Commit submodule configuration
git add .gitmodules third-party/
git commit -m "feat: configure git submodules for open source components

- Added libnetlog as submodule for core parsing functionality
- Added plugin-sdk for extensible plugin architecture  
- Added parsers for community-contributed device support

Enables seamless integration of open source components
with commercial core while maintaining clear licensing boundaries."

git push origin main
```

## 🔧 Step 4: Configure Repository Settings

### 4.1 Branch Protection Rules
For each repository, set up branch protection:

```bash
# Example for main branch protection
gh api repos/NetLogAI/libnetlog/branches/main/protection \
  --method PUT \
  --field required_status_checks='{"strict":true,"contexts":["build-and-test"]}' \
  --field enforce_admins=true \
  --field required_pull_request_reviews='{"required_approving_review_count":1}' \
  --field restrictions=null
```

### 4.2 Repository Topics and Labels
```bash
# Add topics to repositories
gh repo edit NetLogAI/libnetlog --add-topic network-engineering,log-analysis,cpp,parsing
gh repo edit NetLogAI/netlogai-plugin-sdk --add-topic plugins,sdk,cpp,extensibility  
gh repo edit NetLogAI/netlogai-parsers --add-topic parsers,lua,community,network-devices
gh repo edit NetLogAI/netlogai-docs --add-topic documentation,guides,tutorials
```

### 4.3 Security Settings
- Enable dependency alerts and security advisories
- Configure secret scanning for private repository
- Set up code scanning with CodeQL
- Enable vulnerability reporting

## 🚀 Step 5: Set Up CI/CD Integration

### 5.1 Secrets and Variables
Configure organization-level secrets:
```bash
# Add GitHub secrets for cross-repository automation
gh secret set SUBMODULE_SYNC_TOKEN --org NetLogAI --body "$TOKEN"
gh secret set DEPLOYMENT_KEY --org NetLogAI --body "$DEPLOYMENT_KEY"
```

### 5.2 Repository Dispatch Events
Set up automated sync between repositories:
- Public repo updates trigger core repository submodule sync
- Release events coordinate versioning across repositories
- Documentation updates trigger website deployment

## 📊 Step 6: Initialize Development Environment

### 6.1 Clone Complete Environment
```bash
# Clone core repository with all submodules
git clone --recursive https://github.com/NetLogAI/netlogai-core.git
cd netlogai-core

# Run development environment setup
./scripts/setup-dev-env.bat
```

### 6.2 Test Build System
```bash
# Test full build
cmake --build build --config Release --parallel

# Run tests  
ctest --test-dir build --output-on-failure

# Test submodule updates
./scripts/update-submodules.bat
```

## 🔐 Step 7: Security and Compliance Setup

### 7.1 API Key Management
- Set up secure API key storage for AI integration
- Configure environment variable templates
- Document key rotation procedures

### 7.2 License Compliance
- Verify license compatibility across repositories
- Set up automated license scanning
- Create license compliance documentation

### 7.3 Access Controls
- Configure team permissions and access levels
- Set up code review requirements
- Enable audit logging

## 📈 Step 8: Monitoring and Analytics

### 8.1 Repository Analytics
- Set up GitHub Insights for all repositories
- Configure dependency tracking
- Monitor community engagement metrics

### 8.2 CI/CD Monitoring
- Set up build status monitoring
- Configure failure notifications
- Track deployment success rates

## 🎯 Step 9: Community Setup

### 9.1 Community Guidelines
- Create CONTRIBUTING.md files for public repositories
- Set up issue and PR templates
- Configure community health files

### 9.2 Documentation Website
- Deploy documentation to docs.netlogai.com
- Set up search functionality
- Configure analytics and feedback systems

## ✅ Step 10: Validation and Testing

### 10.1 End-to-End Testing
```bash
# Test complete workflow
git clone --recursive https://github.com/NetLogAI/netlogai-core.git test-deployment
cd test-deployment
./scripts/setup-dev-env.bat
cmake --build build --config Release
./build/Release/netlogai.exe --version
```

### 10.2 Integration Validation
- Test plugin loading from SDK
- Validate parser integration
- Verify AI provider configuration
- Test documentation website

## 📋 Post-Deployment Checklist

- [ ] All repositories created with correct visibility
- [ ] Branch protection rules configured
- [ ] CI/CD pipelines passing
- [ ] Submodules properly configured
- [ ] Security scanning enabled
- [ ] Documentation website deployed
- [ ] Community guidelines in place
- [ ] Team access configured
- [ ] License compliance verified
- [ ] API key management documented

## 🆘 Troubleshooting

### Common Issues

**Submodule Update Failures**
```bash
# Reset submodules
git submodule deinit --all -f
git submodule init
git submodule update --recursive
```

**Build Configuration Issues**
```bash
# Clean and reconfigure
rm -rf build/
./scripts/setup-dev-env.bat
```

**Permission Errors**
- Verify GitHub token permissions
- Check organization member roles
- Validate repository access settings

## 📞 Support

- **Technical Issues**: Create issues in respective repositories
- **Organization Setup**: Contact GitHub support
- **Development Questions**: Join Discord community

---

**🎉 Congratulations! Your NetLogAI multi-repository architecture is now ready for development and community collaboration!**