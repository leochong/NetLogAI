@echo off
echo ===============================================
echo NetLogAI Core Development Environment Setup
echo ===============================================
echo.

REM Check for required tools
echo [1/6] Checking prerequisites...

where git >nul 2>nul
if %errorlevel% neq 0 (
    echo ERROR: Git is not installed or not in PATH
    echo Please install Git from https://git-scm.com/
    pause
    exit /b 1
)

where cmake >nul 2>nul
if %errorlevel% neq 0 (
    echo ERROR: CMake is not installed or not in PATH
    echo Please install CMake from https://cmake.org/
    pause
    exit /b 1
)

where cl >nul 2>nul
if %errorlevel% neq 0 (
    echo ERROR: MSVC compiler not found
    echo Please install Visual Studio 2022 with C++ workload
    echo Or run this from a Visual Studio Developer Command Prompt
    pause
    exit /b 1
)

echo ✓ Prerequisites check passed

REM Initialize and update submodules
echo.
echo [2/6] Initializing git submodules...
git submodule init
git submodule update --recursive

if not exist "third-party\libnetlog\CMakeLists.txt" (
    echo ERROR: Failed to initialize libnetlog submodule
    echo Please check your network connection and Git access to public repositories
    pause
    exit /b 1
)

echo ✓ Git submodules initialized

REM Setup vcpkg if not exists
echo.
echo [3/6] Setting up vcpkg package manager...

if not exist "vcpkg" (
    echo Cloning vcpkg...
    git clone https://github.com/Microsoft/vcpkg.git vcpkg
    
    echo Bootstrapping vcpkg...
    call vcpkg\bootstrap-vcpkg.bat
    
    echo Integrating vcpkg...
    vcpkg\vcpkg.exe integrate install
) else (
    echo ✓ vcpkg already exists, updating...
    cd vcpkg
    git pull
    cd ..
)

echo ✓ vcpkg setup complete

REM Install dependencies
echo.
echo [4/6] Installing dependencies with vcpkg...
echo This may take several minutes on first run...

vcpkg\vcpkg.exe install --triplet=x64-windows ^
    nlohmann-json ^
    spdlog ^
    fmt ^
    lua ^
    curl ^
    openssl

if %errorlevel% neq 0 (
    echo ERROR: Failed to install some dependencies
    echo Please check the vcpkg output above for specific errors
    pause
    exit /b 1
)

echo ✓ Dependencies installed successfully

REM Create build directory and configure
echo.
echo [5/6] Configuring CMake build system...

if not exist "build" mkdir build

cmake -B build -S . ^
    -DCMAKE_BUILD_TYPE=Debug ^
    -DCMAKE_TOOLCHAIN_FILE=%CD%\vcpkg\scripts\buildsystems\vcpkg.cmake ^
    -DVCPKG_TARGET_TRIPLET=x64-windows

if %errorlevel% neq 0 (
    echo ERROR: CMake configuration failed
    echo Please check the CMake output above for specific errors
    pause
    exit /b 1
)

echo ✓ CMake configuration complete

REM Build the project
echo.
echo [6/6] Building NetLogAI Core (Debug configuration)...

cmake --build build --config Debug --parallel 4

if %errorlevel% neq 0 (
    echo ERROR: Build failed
    echo Please check the build output above for specific errors
    pause
    exit /b 1
)

echo ✓ Build completed successfully

REM Setup complete
echo.
echo ===============================================
echo Development Environment Setup Complete!
echo ===============================================
echo.
echo Next steps:
echo 1. Open the project in Visual Studio Code
echo 2. Install the C/C++ and CMake Tools extensions
echo 3. Configure your AI provider API keys:
echo    .\build\Debug\netlogai.exe setup ai --provider anthropic
echo.
echo Build commands:
echo   Debug build:     cmake --build build --config Debug
echo   Release build:   cmake --build build --config Release
echo   Commercial build: cmake --build build --config Commercial
echo.
echo Test command:
echo   ctest --test-dir build --output-on-failure
echo.
echo Happy coding! 🚀
pause