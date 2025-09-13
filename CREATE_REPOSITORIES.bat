@echo off
echo ===============================================
echo NetLogAI Multi-Repository Setup Script
echo ===============================================
echo.

REM Check prerequisites
echo [1/8] Checking prerequisites...
where gh >nul 2>nul
if %errorlevel% neq 0 (
    echo ERROR: GitHub CLI (gh) is not installed
    echo Please install from: https://cli.github.com/
    pause
    exit /b 1
)

where git >nul 2>nul
if %errorlevel% neq 0 (
    echo ERROR: Git is not installed
    echo Please install from: https://git-scm.com/
    pause
    exit /b 1
)

REM Check GitHub authentication
echo Checking GitHub authentication...
gh auth status >nul 2>nul
if %errorlevel% neq 0 (
    echo ERROR: Not authenticated with GitHub
    echo Please run: gh auth login
    pause
    exit /b 1
)

echo ✓ Prerequisites check passed

REM Create GitHub organization (manual step)
echo.
echo [2/8] GitHub Organization Setup
echo ===============================================
echo.
echo MANUAL STEP REQUIRED:
echo 1. Go to https://github.com/organizations/new
echo 2. Create organization: NetLogAI
echo 3. Set organization email: admin@netlogai.com
echo 4. Choose "My personal account" plan
echo 5. Complete organization setup
echo.
echo Press any key when organization is created...
pause

REM Create repositories
echo.
echo [3/8] Creating repositories...

echo Creating netlogai-core (PRIVATE)...
gh repo create NetLogAI/netlogai-core --private ^
    --description "NetLogAI Core - Enterprise Network Log Analysis Platform" ^
    --gitignore VisualStudio ^
    --license "" || echo "Repository may already exist"

echo Creating libnetlog (PUBLIC - MIT)...
gh repo create NetLogAI/libnetlog --public ^
    --description "Open Source Network Log Parsing Library" ^
    --gitignore C++ ^
    --license mit || echo "Repository may already exist"

echo Creating netlogai-plugin-sdk (PUBLIC - MIT)...  
gh repo create NetLogAI/netlogai-plugin-sdk --public ^
    --description "NetLogAI Plugin Development Framework" ^
    --gitignore C++ ^
    --license mit || echo "Repository may already exist"

echo Creating netlogai-parsers (PUBLIC - MIT)...
gh repo create NetLogAI/netlogai-parsers --public ^
    --description "Community-Contributed Network Device Parsers" ^
    --gitignore "" ^
    --license mit || echo "Repository may already exist"

echo Creating netlogai-docs (PUBLIC - Creative Commons)...
gh repo create NetLogAI/netlogai-docs --public ^
    --description "Comprehensive Documentation for NetLogAI Ecosystem" ^
    --gitignore Node ^
    --license "" || echo "Repository may already exist"

echo ✓ Repository creation completed

REM Clone and populate repositories
echo.
echo [4/8] Cloning and populating repositories...
mkdir NetLogAI-Deploy 2>nul
cd NetLogAI-Deploy

echo Cloning netlogai-core...
gh repo clone NetLogAI/netlogai-core
cd netlogai-core
xcopy /E /I /Y "..\..\netlogai-core\*" .
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
cd ..

echo Cloning libnetlog...
gh repo clone NetLogAI/libnetlog
cd libnetlog
xcopy /E /I /Y "..\..\libnetlog\*" .
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
cd ..

echo Cloning netlogai-plugin-sdk...
gh repo clone NetLogAI/netlogai-plugin-sdk
cd netlogai-plugin-sdk
xcopy /E /I /Y "..\..\netlogai-plugin-sdk\*" .
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
cd ..

echo Cloning netlogai-parsers...
gh repo clone NetLogAI/netlogai-parsers  
cd netlogai-parsers
xcopy /E /I /Y "..\..\netlogai-parsers\*" .
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
cd ..

echo Cloning netlogai-docs...
gh repo clone NetLogAI/netlogai-docs
cd netlogai-docs
xcopy /E /I /Y "..\..\netlogai-docs\*" .
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
cd ..

echo ✓ Repository population completed

REM Set up submodules in core repository
echo.
echo [5/8] Setting up submodules in core repository...
cd netlogai-core

git submodule add https://github.com/NetLogAI/libnetlog.git third-party/libnetlog
git submodule add https://github.com/NetLogAI/netlogai-plugin-sdk.git third-party/plugin-sdk
git submodule add https://github.com/NetLogAI/netlogai-parsers.git third-party/parsers

git add .gitmodules third-party/
git commit -m "feat: configure git submodules for open source components

- Added libnetlog as submodule for core parsing functionality
- Added plugin-sdk for extensible plugin architecture  
- Added parsers for community-contributed device support

Enables seamless integration of open source components
with commercial core while maintaining clear licensing boundaries."
git push origin main

cd ..

echo ✓ Submodule configuration completed

REM Configure repository settings
echo.
echo [6/8] Configuring repository settings...

echo Adding repository topics...
gh repo edit NetLogAI/libnetlog --add-topic network-engineering,log-analysis,cpp,parsing,cisco,juniper
gh repo edit NetLogAI/netlogai-plugin-sdk --add-topic plugins,sdk,cpp,extensibility,security,performance  
gh repo edit NetLogAI/netlogai-parsers --add-topic parsers,lua,community,network-devices,cisco,juniper
gh repo edit NetLogAI/netlogai-docs --add-topic documentation,guides,tutorials,api-reference
gh repo edit NetLogAI/netlogai-core --add-topic enterprise,cli,ai-integration,network-analysis

echo ✓ Repository configuration completed

REM Test build system
echo.
echo [7/8] Testing build system...
cd netlogai-core

echo Initializing submodules...
git submodule init
git submodule update --recursive

echo Testing development environment setup...
call scripts\setup-dev-env.bat

if %errorlevel% neq 0 (
    echo WARNING: Development environment setup encountered issues
    echo Please review the output above and fix any problems
) else (
    echo ✓ Development environment setup successful
)

cd ..\..

REM Final summary
echo.
echo [8/8] Deployment Summary
echo ===============================================
echo.
echo ✅ COMPLETED TASKS:
echo - Created 5 repositories under NetLogAI organization
echo - Populated repositories with professional structure
echo - Configured git submodules for integration
echo - Set up comprehensive CI/CD workflows
echo - Applied repository topics and metadata
echo - Tested development environment setup
echo.
echo 📋 MANUAL TASKS REMAINING:
echo 1. Configure branch protection rules:
echo    - Go to each repository → Settings → Branches
echo    - Protect main branch with required status checks
echo.
echo 2. Set up organization secrets:
echo    - Go to NetLogAI organization → Settings → Secrets
echo    - Add SUBMODULE_SYNC_TOKEN for automated updates
echo.
echo 3. Enable GitHub Pages for documentation:
echo    - Go to netlogai-docs → Settings → Pages  
echo    - Set source to GitHub Actions
echo.
echo 4. Configure team permissions:
echo    - Add team members to NetLogAI organization
echo    - Set appropriate repository access levels
echo.
echo 🔗 REPOSITORY URLS:
echo - Core (Private):     https://github.com/NetLogAI/netlogai-core
echo - Library (Public):   https://github.com/NetLogAI/libnetlog
echo - Plugin SDK (Public): https://github.com/NetLogAI/netlogai-plugin-sdk
echo - Parsers (Public):   https://github.com/NetLogAI/netlogai-parsers
echo - Documentation:      https://github.com/NetLogAI/netlogai-docs
echo.
echo 🚀 NEXT STEPS:
echo 1. Complete manual configuration tasks above
echo 2. Review deployment guide: DEPLOYMENT_GUIDE.md  
echo 3. Start development with: cd NetLogAI-Deploy\netlogai-core
echo 4. Invite team members to the organization
echo.
echo ===============================================
echo NetLogAI Multi-Repository Setup Complete! 🎉
echo ===============================================
pause