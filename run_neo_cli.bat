@echo off
echo Starting Neo C++ CLI...
echo.

REM Check if build directory exists
if not exist "build" (
    echo Error: Build directory not found. Please build the project first.
    echo Run: cmake --build build --config Release
    pause
    exit /b 1
)

REM Check if CLI executable exists
if not exist "build\apps\cli\Release\neo-cli.exe" (
    echo Error: neo-cli.exe not found. Please build the project first.
    echo Run: cmake --build build --config Release
    pause
    exit /b 1
)

REM Create data directory if it doesn't exist
if not exist "data" mkdir data

REM Copy config file to build directory if it doesn't exist
if not exist "build\apps\cli\Release\config.json" (
    copy "apps\cli\config.json" "build\apps\cli\Release\config.json"
)

REM Change to the CLI directory
cd build\apps\cli\Release

REM Run the CLI
echo Starting Neo CLI...
echo Type 'help' for available commands
echo Type 'exit' to quit
echo.
neo-cli.exe

REM Return to original directory
cd ..\..\..\..

echo.
echo Neo CLI has exited.
pause 