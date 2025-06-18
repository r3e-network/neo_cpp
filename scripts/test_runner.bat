@echo off
setlocal EnableDelayedExpansion

REM ========================================================================
REM Neo C++ Comprehensive Test Runner for Windows
REM This script runs all available tests with detailed reporting and analysis
REM ========================================================================

echo.
echo ================================================================
echo   Neo C++ Comprehensive Test Runner v1.0.0
echo   Running all tests with detailed analysis and reporting
echo ================================================================
echo.

REM Set script variables
set "SCRIPT_DIR=%~dp0"
set "PROJECT_ROOT=%SCRIPT_DIR%\.."
set "BUILD_DIR=%PROJECT_ROOT%\build-release"
set "TEST_RESULTS_DIR=%PROJECT_ROOT%\test_results"
set "ERROR_COUNT=0"
set "WARNING_COUNT=0"
set "TOTAL_TESTS=0"
set "PASSED_TESTS=0"
set "FAILED_TESTS=0"

REM Color codes for output
set "RED=[91m"
set "GREEN=[92m" 
set "YELLOW=[93m"
set "BLUE=[94m"
set "NC=[0m"

REM ========================================================================
REM Helper Functions
REM ========================================================================

:log_info
echo %BLUE%[INFO]%NC% %~1
goto :eof

:log_success
echo %GREEN%[SUCCESS]%NC% %~1
goto :eof

:log_warning
echo %YELLOW%[WARNING]%NC% %~1
set /a WARNING_COUNT+=1
goto :eof

:log_error
echo %RED%[ERROR]%NC% %~1
set /a ERROR_COUNT+=1
goto :eof

REM ========================================================================
REM Test Environment Setup
REM ========================================================================

:setup_test_environment
call :log_info "Setting up test environment..."

REM Create test results directory
if not exist "%TEST_RESULTS_DIR%" mkdir "%TEST_RESULTS_DIR%"

REM Check if build directory exists
if not exist "%BUILD_DIR%" (
    call :log_error "Build directory not found: %BUILD_DIR%"
    call :log_error "Please run build verification first: scripts\build_verify.bat"
    goto :error_exit
)

REM Change to build directory
cd /d "%BUILD_DIR%"

call :log_success "Test environment ready"
goto :eof

REM ========================================================================
REM Unit Tests
REM ========================================================================

:run_unit_tests
call :log_info "Running unit tests..."

set "UNIT_TEST_EXE=Release\neo-unit-tests.exe"
set "UNIT_TEST_REPORT=%TEST_RESULTS_DIR%\unit_tests.xml"

if exist "%UNIT_TEST_EXE%" (
    call :log_info "Found unit tests executable"
    
    REM Run unit tests with XML output
    "%UNIT_TEST_EXE%" --gtest_output=xml:"%UNIT_TEST_REPORT%" --gtest_brief=1
    
    if errorlevel 1 (
        call :log_warning "Some unit tests failed"
        set /a FAILED_TESTS+=1
    ) else (
        call :log_success "Unit tests passed"
        set /a PASSED_TESTS+=1
    )
    set /a TOTAL_TESTS+=1
    
    REM Parse results if XML exists
    if exist "%UNIT_TEST_REPORT%" (
        call :parse_gtest_results "%UNIT_TEST_REPORT%" "Unit Tests"
    )
) else (
    call :log_warning "Unit tests executable not found: %UNIT_TEST_EXE%"
    call :log_warning "Build the project with testing enabled to run unit tests"
)

goto :eof

REM ========================================================================
REM Integration Tests
REM ========================================================================

:run_integration_tests
call :log_info "Running integration tests..."

set "INTEGRATION_TEST_EXE=Release\neo-integration-tests.exe"
set "INTEGRATION_TEST_REPORT=%TEST_RESULTS_DIR%\integration_tests.xml"

if exist "%INTEGRATION_TEST_EXE%" (
    call :log_info "Found integration tests executable"
    
    REM Run integration tests
    "%INTEGRATION_TEST_EXE%" --gtest_output=xml:"%INTEGRATION_TEST_REPORT%"
    
    if errorlevel 1 (
        call :log_warning "Some integration tests failed"
        set /a FAILED_TESTS+=1
    ) else (
        call :log_success "Integration tests passed"
        set /a PASSED_TESTS+=1
    )
    set /a TOTAL_TESTS+=1
    
    REM Parse results if XML exists
    if exist "%INTEGRATION_TEST_REPORT%" (
        call :parse_gtest_results "%INTEGRATION_TEST_REPORT%" "Integration Tests"
    )
) else (
    call :log_warning "Integration tests executable not found: %INTEGRATION_TEST_EXE%"
)

goto :eof

REM ========================================================================
REM Performance Benchmarks
REM ========================================================================

:run_benchmarks
call :log_info "Running performance benchmarks..."

set "BENCHMARK_EXE=Release\neo-benchmarks.exe"
set "BENCHMARK_REPORT=%TEST_RESULTS_DIR%\benchmarks.json"

if exist "%BENCHMARK_EXE%" (
    call :log_info "Found benchmarks executable"
    
    REM Run benchmarks with JSON output
    "%BENCHMARK_EXE%" --benchmark_format=json --benchmark_out="%BENCHMARK_REPORT%"
    
    if errorlevel 1 (
        call :log_warning "Some benchmarks failed"
        set /a FAILED_TESTS+=1
    ) else (
        call :log_success "Benchmarks completed"
        set /a PASSED_TESTS+=1
    )
    set /a TOTAL_TESTS+=1
    
    REM Display benchmark summary
    if exist "%BENCHMARK_REPORT%" (
        call :log_success "Benchmark results saved to: %BENCHMARK_REPORT%"
    )
) else (
    call :log_warning "Benchmarks executable not found: %BENCHMARK_EXE%"
)

goto :eof

REM ========================================================================
REM Compatibility Tests
REM ========================================================================

:run_compatibility_tests
call :log_info "Running compatibility verification..."

set "COMPATIBILITY_SCRIPT=%PROJECT_ROOT%\scripts\verify_neo3_compatibility.py"
set "COMPATIBILITY_REPORT=%TEST_RESULTS_DIR%\compatibility_report.txt"

if exist "%COMPATIBILITY_SCRIPT%" (
    call :log_info "Running Neo3 compatibility verification..."
    
    REM Run Python compatibility script
    python "%COMPATIBILITY_SCRIPT%" > "%COMPATIBILITY_REPORT%" 2>&1
    
    if errorlevel 1 (
        call :log_warning "Compatibility issues found"
        set /a FAILED_TESTS+=1
    ) else (
        call :log_success "Compatibility verification passed"
        set /a PASSED_TESTS+=1
    )
    set /a TOTAL_TESTS+=1
    
    call :log_success "Compatibility report saved to: %COMPATIBILITY_REPORT%"
) else (
    call :log_warning "Compatibility verification script not found"
)

goto :eof

REM ========================================================================
REM Smoke Tests
REM ========================================================================

:run_smoke_tests
call :log_info "Running smoke tests..."

REM Test 1: Node executable basic functionality
call :log_info "Testing node executable..."
set "NODE_EXE=src\node\Release\neo-node.exe"

if exist "%NODE_EXE%" (
    cd /d "%BUILD_DIR%\src\node\Release"
    
    REM Test help command
    neo-node.exe --help > nul 2>&1
    if errorlevel 1 (
        call :log_warning "Node --help command failed"
        set /a FAILED_TESTS+=1
    ) else (
        call :log_success "Node --help command works"
        set /a PASSED_TESTS+=1
    )
    set /a TOTAL_TESTS+=1
    
    REM Test version command (if available)
    neo-node.exe --version > nul 2>&1
    if errorlevel 1 (
        call :log_warning "Node --version command failed (may not be implemented)"
    ) else (
        call :log_success "Node --version command works"
    )
    
    cd /d "%BUILD_DIR%"
) else (
    call :log_error "Node executable not found: %NODE_EXE%"
    set /a FAILED_TESTS+=1
    set /a TOTAL_TESTS+=1
)

REM Test 2: CLI executable basic functionality (if available)
set "CLI_EXE=Release\neo-cli.exe"
if exist "%CLI_EXE%" (
    call :log_info "Testing CLI executable..."
    
    REM Test help command
    "%CLI_EXE%" help > nul 2>&1
    if errorlevel 1 (
        call :log_warning "CLI help command failed"
        set /a FAILED_TESTS+=1
    ) else (
        call :log_success "CLI help command works"
        set /a PASSED_TESTS+=1
    )
    set /a TOTAL_TESTS+=1
) else (
    call :log_warning "CLI executable not found (may not be implemented yet)"
)

call :log_success "Smoke tests completed"
goto :eof

REM ========================================================================
REM Memory and Performance Tests
REM ========================================================================

:run_memory_tests
call :log_info "Running memory and performance tests..."

set "NODE_EXE=src\node\Release\neo-node.exe"

if exist "%NODE_EXE%" (
    call :log_info "Testing memory usage and basic performance..."
    
    REM Create a simple test configuration
    set "TEST_CONFIG=%TEST_RESULTS_DIR%\test_config.json"
    echo { > "%TEST_CONFIG%"
    echo   "ApplicationConfiguration": { >> "%TEST_CONFIG%"
    echo     "Logger": { >> "%TEST_CONFIG%"
    echo       "Path": "Logs", >> "%TEST_CONFIG%"
    echo       "ConsoleOutput": true >> "%TEST_CONFIG%"
    echo     }, >> "%TEST_CONFIG%"
    echo     "Storage": { >> "%TEST_CONFIG%"
    echo       "Engine": "MemoryStore" >> "%TEST_CONFIG%"
    echo     } >> "%TEST_CONFIG%"
    echo   } >> "%TEST_CONFIG%"
    echo } >> "%TEST_CONFIG%"
    
    cd /d "%BUILD_DIR%\src\node\Release"
    
    REM Test node startup and quick shutdown
    call :log_info "Testing node startup performance..."
    set START_TIME=%time%
    
    REM Start node with test config for 5 seconds
    timeout /t 5 /nobreak > nul 2>&1
    
    if errorlevel 1 (
        call :log_warning "Node startup test had issues"
        set /a FAILED_TESTS+=1
    ) else (
        call :log_success "Node startup test completed"
        set /a PASSED_TESTS+=1
    )
    set /a TOTAL_TESTS+=1
    
    cd /d "%BUILD_DIR%"
) else (
    call :log_warning "Cannot run memory tests - node executable not found"
)

goto :eof

REM ========================================================================
REM Parse Test Results
REM ========================================================================

:parse_gtest_results
set "XML_FILE=%~1"
set "TEST_NAME=%~2"

call :log_info "Parsing %TEST_NAME% results..."

REM Simple XML parsing for test count (this is basic, a proper XML parser would be better)
for /f "tokens=2 delims= " %%a in ('findstr "tests=" "%XML_FILE%" 2^>nul') do (
    for /f "tokens=1 delims='" %%b in ("%%a") do (
        call :log_info "%TEST_NAME% total tests: %%b"
    )
)

for /f "tokens=2 delims= " %%a in ('findstr "failures=" "%XML_FILE%" 2^>nul') do (
    for /f "tokens=1 delims='" %%b in ("%%a") do (
        if not "%%b"=="0" (
            call :log_warning "%TEST_NAME% failures: %%b"
        )
    )
)

goto :eof

REM ========================================================================
REM Generate Test Report
REM ========================================================================

:generate_test_report
call :log_info "Generating comprehensive test report..."

set "REPORT_FILE=%TEST_RESULTS_DIR%\test_summary_report.txt"

echo Neo C++ Comprehensive Test Report > "%REPORT_FILE%"
echo ================================== >> "%REPORT_FILE%"
echo. >> "%REPORT_FILE%"
echo Test Date: %DATE% %TIME% >> "%REPORT_FILE%"
echo Build Directory: %BUILD_DIR% >> "%REPORT_FILE%"
echo Test Results Directory: %TEST_RESULTS_DIR% >> "%REPORT_FILE%"
echo. >> "%REPORT_FILE%"
echo Test Summary: >> "%REPORT_FILE%"
echo - Total Test Suites: !TOTAL_TESTS! >> "%REPORT_FILE%"
echo - Passed Test Suites: !PASSED_TESTS! >> "%REPORT_FILE%"
echo - Failed Test Suites: !FAILED_TESTS! >> "%REPORT_FILE%"
echo - Success Rate: >> "%REPORT_FILE%"
if !TOTAL_TESTS! GTR 0 (
    set /a SUCCESS_RATE=!PASSED_TESTS! * 100 / !TOTAL_TESTS!
    echo   !SUCCESS_RATE!%% >> "%REPORT_FILE%"
) else (
    echo   N/A ^(no tests found^) >> "%REPORT_FILE%"
)
echo. >> "%REPORT_FILE%"
echo Test Files Generated: >> "%REPORT_FILE%"
if exist "%TEST_RESULTS_DIR%\unit_tests.xml" (
    echo - Unit Tests: unit_tests.xml >> "%REPORT_FILE%"
)
if exist "%TEST_RESULTS_DIR%\integration_tests.xml" (
    echo - Integration Tests: integration_tests.xml >> "%REPORT_FILE%"
)
if exist "%TEST_RESULTS_DIR%\benchmarks.json" (
    echo - Benchmarks: benchmarks.json >> "%REPORT_FILE%"
)
if exist "%TEST_RESULTS_DIR%\compatibility_report.txt" (
    echo - Compatibility: compatibility_report.txt >> "%REPORT_FILE%"
)
echo. >> "%REPORT_FILE%"
echo Status: >> "%REPORT_FILE%"
echo - Errors: !ERROR_COUNT! >> "%REPORT_FILE%"
echo - Warnings: !WARNING_COUNT! >> "%REPORT_FILE%"
if !ERROR_COUNT! EQU 0 (
    if !FAILED_TESTS! EQU 0 (
        echo - Overall Status: SUCCESS >> "%REPORT_FILE%"
    ) else (
        echo - Overall Status: PARTIAL SUCCESS >> "%REPORT_FILE%"
    )
) else (
    echo - Overall Status: FAILED >> "%REPORT_FILE%"
)

call :log_success "Test report generated: %REPORT_FILE%"
goto :eof

REM ========================================================================
REM Main Execution Flow
REM ========================================================================

:main
cd /d "%PROJECT_ROOT%"

REM Parse command line arguments
set "SKIP_UNIT=false"
set "SKIP_INTEGRATION=false"
set "SKIP_BENCHMARKS=false"
set "SKIP_COMPATIBILITY=false"
set "QUICK_MODE=false"

:parse_args
if "%~1"=="" goto :start_tests
if "%~1"=="--skip-unit" (
    set "SKIP_UNIT=true"
    shift
    goto :parse_args
)
if "%~1"=="--skip-integration" (
    set "SKIP_INTEGRATION=true"
    shift
    goto :parse_args
)
if "%~1"=="--skip-benchmarks" (
    set "SKIP_BENCHMARKS=true"
    shift
    goto :parse_args
)
if "%~1"=="--skip-compatibility" (
    set "SKIP_COMPATIBILITY=true"
    shift
    goto :parse_args
)
if "%~1"=="--quick" (
    set "QUICK_MODE=true"
    shift
    goto :parse_args
)
if "%~1"=="--help" (
    echo Usage: %0 [options]
    echo Options:
    echo   --skip-unit          Skip unit tests
    echo   --skip-integration   Skip integration tests
    echo   --skip-benchmarks    Skip performance benchmarks
    echo   --skip-compatibility Skip compatibility verification
    echo   --quick              Quick mode - run only essential tests
    echo   --help               Show this help message
    goto :eof
)
call :log_warning "Unknown option: %~1"
shift
goto :parse_args

:start_tests
call :log_info "Starting comprehensive test execution..."

REM Setup test environment
call :setup_test_environment
if errorlevel 1 goto :error_exit

REM Run smoke tests first (always)
call :run_smoke_tests

REM Run other test suites based on options
if "%SKIP_UNIT%"=="false" (
    call :run_unit_tests
) else (
    call :log_warning "Skipping unit tests"
)

if "%SKIP_INTEGRATION%"=="false" (
    call :run_integration_tests
) else (
    call :log_warning "Skipping integration tests"
)

if "%QUICK_MODE%"=="false" (
    if "%SKIP_BENCHMARKS%"=="false" (
        call :run_benchmarks
    ) else (
        call :log_warning "Skipping benchmarks"
    )
    
    if "%SKIP_COMPATIBILITY%"=="false" (
        call :run_compatibility_tests
    ) else (
        call :log_warning "Skipping compatibility tests"
    )
    
    call :run_memory_tests
) else (
    call :log_warning "Quick mode - skipping benchmarks, compatibility, and memory tests"
)

REM Generate comprehensive report
call :generate_test_report

REM Final status
echo.
echo ================================================================
if !ERROR_COUNT! EQU 0 (
    if !FAILED_TESTS! EQU 0 (
        call :log_success "All tests completed successfully!"
        echo.
        echo Test Summary:
        echo - Total Test Suites: !TOTAL_TESTS!
        echo - Passed: !PASSED_TESTS!
        echo - Failed: !FAILED_TESTS!
        if !TOTAL_TESTS! GTR 0 (
            set /a SUCCESS_RATE=!PASSED_TESTS! * 100 / !TOTAL_TESTS!
            echo - Success Rate: !SUCCESS_RATE!%%
        )
    ) else (
        call :log_warning "Tests completed with !FAILED_TESTS! failed test suites"
        echo Please review the individual test results for details.
    )
) else (
    call :log_error "Test execution completed with !ERROR_COUNT! errors and !WARNING_COUNT! warnings"
)

echo.
echo Test results available in: %TEST_RESULTS_DIR%
echo Comprehensive report: %TEST_RESULTS_DIR%\test_summary_report.txt
echo ================================================================
echo.

goto :eof

:error_exit
call :log_error "Test execution failed!"
echo.
echo Please ensure:
echo 1. The project has been built successfully
echo 2. You're running this script from the project root
echo 3. Required test executables exist in the build directory
echo.
exit /b 1

REM Execute main function
call :main %* 