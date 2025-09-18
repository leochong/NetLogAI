@echo off
echo NetLogAI Log File Testing Suite
echo ================================

set NETLOGAI_EXE=.\netlogai-core\build\Release\netlogai.exe
set TEST_DIR=test-logs

echo.
echo Testing NetLogAI binary exists...
if not exist "%NETLOGAI_EXE%" (
    echo ERROR: NetLogAI executable not found at %NETLOGAI_EXE%
    echo Please build the project first: cmake --build build --config Release
    exit /b 1
)

echo.
echo Testing Cisco IOS BGP logs...
"%NETLOGAI_EXE%" analyze "%TEST_DIR%\cisco-ios\bgp\bgp_neighbor_down.log"

echo.
echo Testing Cisco IOS Interface logs...
"%NETLOGAI_EXE%" analyze "%TEST_DIR%\cisco-ios\interface\interface_flap.log"

echo.
echo Testing Cisco ASA Firewall logs...
"%NETLOGAI_EXE%" analyze "%TEST_DIR%\cisco-asa\security\firewall_blocks.log"

echo.
echo Testing parser validation...
"%NETLOGAI_EXE%" parser test --directory "%TEST_DIR%"

echo.
echo All tests completed!
pause