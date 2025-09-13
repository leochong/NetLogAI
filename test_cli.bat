@echo off
echo Testing NetLogAI CLI Application
echo =================================

echo.
echo 1. Testing --help flag:
echo --------------------------
netlogai-core\build\Release\netlogai.exe --help

echo.
echo 2. Testing status command:
echo --------------------------
netlogai-core\build\Release\netlogai.exe status

echo.
echo 3. Testing parser list command:
echo --------------------------
netlogai-core\build\Release\netlogai.exe parser list

echo.
echo 4. Testing config init command:
echo --------------------------
netlogai-core\build\Release\netlogai.exe config init

echo.
echo CLI Application Testing Complete!