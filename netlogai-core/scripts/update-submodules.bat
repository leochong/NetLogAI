@echo off
echo ===============================================
echo NetLogAI Core - Submodule Update Script
echo ===============================================
echo.

REM Check if we're in a git repository
if not exist ".git" (
    echo ERROR: This script must be run from the root of the netlogai-core repository
    pause
    exit /b 1
)

echo [1/5] Updating git submodules to latest versions...
echo.

REM Update all submodules to their latest versions
echo Fetching latest changes from submodule repositories...
git submodule foreach git fetch origin

echo.
echo Updating submodules to latest commits on their main branches...

REM Update libnetlog submodule
if exist "third-party\libnetlog\.git" (
    echo [2/5] Updating libnetlog submodule...
    cd third-party\libnetlog
    git checkout main
    git pull origin main
    cd ..\..
    echo ✓ libnetlog updated successfully
) else (
    echo ERROR: libnetlog submodule not found
    echo Please run 'git submodule init && git submodule update' first
    pause
    exit /b 1
)

REM Update plugin-sdk submodule
if exist "third-party\plugin-sdk\.git" (
    echo [3/5] Updating plugin-sdk submodule...
    cd third-party\plugin-sdk
    git checkout main
    git pull origin main
    cd ..\..
    echo ✓ plugin-sdk updated successfully
) else (
    echo WARNING: plugin-sdk submodule not found - this is optional
)

REM Update parsers submodule
if exist "third-party\parsers\.git" (
    echo [4/5] Updating parsers submodule...
    cd third-party\parsers
    git checkout main
    git pull origin main
    cd ..\..
    echo ✓ parsers updated successfully
) else (
    echo WARNING: parsers submodule not found - this is optional
)

echo.
echo [5/5] Committing submodule updates...

REM Check if there are any changes to commit
git diff --quiet HEAD
if %errorlevel% equ 0 (
    echo No submodule updates to commit
) else (
    echo Committing submodule updates to netlogai-core...
    git add third-party/
    git commit -m "chore: update submodules to latest versions

    - Updated libnetlog to latest main branch
    - Updated plugin-sdk to latest main branch  
    - Updated parsers to latest main branch
    
    🤖 Generated with NetLogAI submodule update script"
    
    echo.
    echo ✓ Submodule updates committed successfully
    echo.
    echo To push these changes, run:
    echo   git push origin main
)

echo.
echo ===============================================
echo Submodule Update Complete!
echo ===============================================
echo.
echo Summary of updated components:
echo - libnetlog: Core log parsing library
echo - plugin-sdk: Plugin development framework
echo - parsers: Community parser collection
echo.
echo Next steps:
echo 1. Test the updated components with: cmake --build build --target test
echo 2. If tests pass, push changes with: git push origin main
echo 3. Create a new release if this is a significant update
echo.
pause